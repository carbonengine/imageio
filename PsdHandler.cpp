#include "StdAfx.h"
#include "PsdHandler.h"
#include "HostBitmap.h"

using namespace Tr2RenderContextEnum;

namespace
{

const uint32_t PSD_SIGNATURE = 0x53504238;
const uint16_t PSD_COLOR_MODE_GRAYSCALE = 1;
const uint16_t PSD_COLOR_MODE_RGB = 3;

uint32_t SwitchEndian( uint32_t x )
{
	return 
		( ( x & 0xff000000 ) >> 24 ) | 
		( ( x & 0xff0000 ) >> 8 ) | 
		( ( x & 0xff00 ) << 8 ) | 
		( ( x & 0xff ) << 24 );
}

uint16_t SwitchEndian( uint16_t x )
{
	return 
		( ( x & 0xff00 ) >> 8 ) | 
		( ( x & 0xff ) << 8 );
}

bool SkipBlock( ICcpStream* stream )
{
    uint32_t tmp;
	if( stream->Read( &tmp, sizeof( tmp ) ) != sizeof( tmp ) )
	{
		return false;
	}
	stream->Seek( SwitchEndian( tmp ), ICcpStream::SO_CURRENT );
	return true;
}

bool WriteZeroLengthBlock( ICcpStream* stream )
{
	uint32_t tmp = 0;
	if( stream->Write( &tmp, sizeof( tmp ) ) != sizeof( tmp ) )
	{
		CCP_LOGWARN( "PsdHandler::Save failed to write header" );
		return false;
	}
	return true;
}

}

namespace ImageIO
{


// --------------------------------------------------------------------------------------
// Description:
//   PsdHandler constructor
// --------------------------------------------------------------------------------------
PsdHandler::PsdHandler( const wchar_t* sourceName )
	:Tr2ImageHandler( sourceName ),
	m_compression( 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads PSD header from the stream and initializes basic variables (width, height, 
//   etc.).
// Arguments:
//   stream - Data stream
// Return Value:
//   true If the header was suscessfully read
//   false If there was an error reading the header or the format is unsupported
// --------------------------------------------------------------------------------------
bool PsdHandler::ReadHeader( ICcpStream* stream )
{
	if( stream->Read( &m_header, sizeof( Header ) ) != sizeof( Header ) )
	{
		return false;
	}
	m_header.width = SwitchEndian( m_header.width );
	m_header.height = SwitchEndian( m_header.height );
	m_header.version = SwitchEndian( m_header.version );
	m_header.channelCount = SwitchEndian( m_header.channelCount );
	m_header.depth = SwitchEndian( m_header.depth );
	m_header.colorMode = SwitchEndian( m_header.colorMode );

	if( m_header.signature != PSD_SIGNATURE || m_header.version != 1 || m_header.channelCount > 4 || 
		m_header.depth != 8 || ( m_header.colorMode != PSD_COLOR_MODE_RGB && m_header.colorMode != PSD_COLOR_MODE_GRAYSCALE ) )
	{
		return false;
	}

	SkipBlock( stream );
	SkipBlock( stream );
	SkipBlock( stream );

    // Find out if the data is compressed.
    // Known values:
    //   0: no compression
    //   1: RLE compressed
 	if( stream->Read( &m_compression, sizeof( m_compression ) ) != sizeof( m_compression ) )
	{
		return false;
	}
	m_compression = SwitchEndian( m_compression );
    if( m_compression > 1 )
	{
        // Unknown compression type.
        return false;
    }

	m_width = m_header.width;
	m_height = m_header.height;
	m_bitsPerPixel = m_header.channelCount == 3 ? 32 : m_header.channelCount * 8;
	m_mipLevelCount = 1;

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the format of the texture being loaded.
//   Assumes that header has been successfully read, and determined to be 
//   valid - only call after ReadHeader returns true.
// Return Value:
//   Pixel format for PSD image
// --------------------------------------------------------------------------------------
Tr2RenderContextEnum::PixelFormat PsdHandler::GetFormat() const 
{
	switch( m_header.channelCount )
	{
	case 1:
		return Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM;
	case 2:
		return Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UNORM;
	case 3:
		return Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
	default:
		return Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM;
	}
}

bool PsdHandler::ReadRleData( ICcpStream* stream )
{
	const uint32_t componentsBgra[] = { 2, 1, 0, 3 };
	const uint32_t componentsR[] = { 0, };
	const uint32_t* components = m_header.colorMode == PSD_COLOR_MODE_GRAYSCALE ? componentsR : componentsBgra;
	const uint32_t bpp = m_header.colorMode == PSD_COLOR_MODE_GRAYSCALE ? 1 : 4;
	size_t channelSize = m_header.width * m_header.height;

	stream->Seek( m_header.height * m_header.channelCount * sizeof( uint16_t ), ICcpStream::SO_CURRENT );
	CcpMallocBuffer compressedData( "PsdHandler::ReadImage/compressedData", stream->GetSize() - stream->GetPosition() );
	if( stream->Read( compressedData.get(), compressedData.size() ) != compressedData.size() )
	{
		return false;
	}
	size_t compressedIndex = 0;

    // Read RLE data.
    for( uint32_t channel = 0; channel < m_header.channelCount; channel++ )
    {
        uint8_t* ptr = m_data + components[channel];

        uint32_t count = 0;
        while( count < channelSize )
        {
			if( compressedIndex >= compressedData.size() )
			{
				return false;
			}

			uint8_t c = reinterpret_cast<uint8_t*>( compressedData.get() )[compressedIndex++];

            uint32_t len = c;
            if (len < 128)
            {
                // Copy next len+1 bytes literally.
                len++;
                count += len;
                if( count > channelSize )
				{
					return false;
				}

                while (len != 0)
                {
					*ptr = reinterpret_cast<uint8_t*>( compressedData.get() )[compressedIndex++];
                    ptr += bpp;
                    len--;
                }
            }
            else if (len > 128)
            {
                // Next -len+1 bytes in the dest are replicated from next source byte.
                // (Interpret len as a negative 8-bit int.)
                len ^= 0xFF;
                len += 2;
                count += len;
                if( compressedIndex >= compressedData.size() || count > channelSize ) 
				{
					return false;
				}

                uint8_t val = reinterpret_cast<uint8_t*>( compressedData.get() )[compressedIndex++];
                while( len != 0 ) 
				{
                    *ptr = val;
                    ptr += bpp;
                    len--;
                }
            }
            else if( len == 128 ) {
                // No-op.
            }
        }
    }
	return true;
}

bool PsdHandler::ReadUncompressedData( ICcpStream* stream )
{
	const uint32_t componentsBgra[] = { 2, 1, 0, 3 };
	const uint32_t componentsR[] = { 0, 1, };
	const uint32_t* components = m_header.channelCount < 3 ? componentsR : componentsBgra;
	const uint32_t bpp = m_header.channelCount == 3 ? 4 : m_header.channelCount;
	size_t channelSize = m_header.width * m_header.height;

	CcpMallocBuffer channel( "PsdHandler::ReadImage/channel", channelSize );
	for( uint32_t component = 0; component < m_header.channelCount; ++component )
	{
		if( stream->Read( channel.get(), channelSize ) != ptrdiff_t( channelSize ) )
		{
			return false;
		}
		for( size_t i = 0; i < channelSize; ++i )
		{
			m_data[i * bpp + components[component]] = reinterpret_cast<const uint8_t*>( channel.get() )[i];
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads image data from the stream.
// Arguments:
//   stream - Data stream
// Return Value:
//   true If the image was suscessfully read
//   false If there was an error loading the image
// --------------------------------------------------------------------------------------
bool PsdHandler::ReadImage( ICcpStream* stream )
{
	CCP_FREE( m_data );
	m_dataSize = GetTotalDataSize();
	m_data = (uint8_t*)CCP_MALLOC( "PsdHandler/m_data", GetTotalDataSize() );
	if( !m_data )
	{
		CCP_LOGERR( "PsdHandler couldn't allocate %d bytes (%S)", GetTotalDataSize(), m_sourceName.c_str() );
		m_dataSize = 0;
		return false;
	}

	if( m_compression == 1 )
	{
		if( !ReadRleData( stream ) )
		{
			return false;
		}
	}
	else
	{
		if( !ReadUncompressedData( stream ) )
		{
			return false;
		}
	}
	if( m_header.channelCount == 3 )
	{
		for( size_t i = 0; i < GetWidth() * GetHeight(); ++i )
		{
			m_data[i * 4 + 3] = 0xff;
		}
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return the dds block size, or 0 if there's no compression
// Return Value:
//   0 always
// --------------------------------------------------------------------------------------
unsigned PsdHandler::GetBlockByteSize() const
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return the offset where this face and miplevel would be, relative to the start of m_data.
//   This does not include the size of the header, ie. we're dealing strictly with texels.
// Arguments:
//   mipLevel - Mip level
//   face - Cubemap face (unused: PSDs don't support cube maps)
// Return Value:
//   Offset of specified mip level.
// --------------------------------------------------------------------------------------
unsigned PsdHandler::GetOffset( unsigned mipLevel, unsigned ) const
{
	unsigned offset = 0;
	for( unsigned int i = 0; i != mipLevel ; ++i )
	{
		offset += GetMipLevelSize( i );
	}
	return offset;
}

bool PsdHandler::IsSaveSupported( const Tr2BitmapDimensions& bd )
{
	if( bd.GetType() != TEX_TYPE_2D || 
		( bd.GetFormat() != PIXEL_FORMAT_R8_UNORM &&
		bd.GetFormat() != PIXEL_FORMAT_R8G8_UNORM &&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM &&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM ) )
	{
		return false;
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves a bitmap to PSD file.
// Arguments:
//   image - Bitmap to save
//   output - Stream to save image to
// Return Value:
//   true If the image was saved
//   false Otherwise
// --------------------------------------------------------------------------------------
bool PsdHandler::Save( const ImageIO::HostBitmap& image, ICcpStream* output )
{
	if( !image.IsValid() )
	{
		CCP_LOGWARN( "PsdHandler::Save input image isn't valid" );
		return false;
	}

	if( !IsSaveSupported( image ) )
	{
		CCP_LOGWARN( "PsdHandler::Save does not support this image format" );
		return false;
	}

	const uint32_t bpp = GetBytesPerPixel( image.GetFormat() );
	uint32_t channelCount = image.GetFormat() == PIXEL_FORMAT_B8G8R8X8_UNORM ? 3 : bpp;

	Header header;
	header.signature = PSD_SIGNATURE;
	header.version = SwitchEndian( uint16_t( 1 ) );
	header.reserved[0] = 0;
	header.reserved[1] = 0;
	header.reserved[2] = 0;
	header.reserved[3] = 0;
	header.reserved[4] = 0;
	header.reserved[5] = 0;
	header.channelCount = SwitchEndian( uint16_t( channelCount ) );
	header.height = SwitchEndian( image.GetHeight() );
	header.width = SwitchEndian( image.GetWidth() );
	header.depth = SwitchEndian( uint16_t( 8 ) );
	header.colorMode = SwitchEndian( channelCount > 2 ? PSD_COLOR_MODE_RGB : PSD_COLOR_MODE_GRAYSCALE );

	if( output->Write( &header, sizeof( Header ) ) != sizeof( Header ) )
	{
		CCP_LOGWARN( "PsdHandler::Save failed to write header" );
		return false;
	}

	WriteZeroLengthBlock( output );
	WriteZeroLengthBlock( output );
	WriteZeroLengthBlock( output );

	uint16_t compression = 0;
	if( output->Write( &compression, sizeof( compression ) ) != sizeof( compression ) )
	{
		CCP_LOGWARN( "PsdHandler::Save failed to write header" );
		return false;
	}

	const uint32_t componentsBgra[] = { 2, 1, 0, 3 };
	const uint32_t componentsR[] = { 0, 1, };
	const uint32_t* components = channelCount < 3 ? componentsR : componentsBgra;
	size_t channelSize = image.GetWidth() * image.GetHeight();
	CcpMallocBuffer channel( "PsdHandler::Save/channel", channelSize );
	for( uint32_t component = 0; component < channelCount; ++component )
	{
		for( size_t i = 0; i < channelSize; ++i )
		{
			channel.get()[i] = image.GetRawData()[i * bpp + components[component]];
		}
		if( output->Write( channel.get(), channelSize ) != channelSize )
		{
			CCP_LOGWARN( "PsdHandler::Save failed to write image data" );
			return false;
		}
	}
	return true;
}


}