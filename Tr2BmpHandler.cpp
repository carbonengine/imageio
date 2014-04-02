////////////////////////////////////////////////////////////
//
//    Creator:   Filipp Pavlov
//    Created:   July 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2BmpHandler.h"
#include "HostBitmap.h"

using namespace Tr2RenderContextEnum;

namespace
{

const unsigned short BMP_SIGNATURE = 0x4D42;
const unsigned NO_COMPRESSION = 0;

}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2BmpHandler constructor
// --------------------------------------------------------------------------------------
Tr2BmpHandler::Tr2BmpHandler( const wchar_t* sourceName )
	:Tr2ImageHandler( sourceName ),
	m_fileSize( 0 ),
	m_imageSize( 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads BMP header from the stream and initializes basic variables (width, height, 
//   etc.).
// Arguments:
//   stream - Data stream
// Return Value:
//   true If the header was suscessfully read
//   false If there was an error reading the header or the format is unsupported
// --------------------------------------------------------------------------------------
bool Tr2BmpHandler::ReadHeader( ICcpStream* stream )
{
	m_fileSize = stream->GetSize();

	if( stream->Read( &m_bmpHeader, sizeof( BmpHeader ) ) != sizeof( BmpHeader ) )
	{
		return false;
	}
	if( m_bmpHeader.type != BMP_SIGNATURE )
	{
		return false;
	}
	if( stream->Read( &m_dibHeader, sizeof( MinDibHeader ) ) != sizeof( MinDibHeader ) )
	{
		return false;
	}
	if( m_dibHeader.bitCount != 24 && m_dibHeader.bitCount != 32 )
	{
		return false;
	}
	if( m_dibHeader.structSize >= sizeof( DibHeader ) )
	{
		if( stream->Read( reinterpret_cast<char*>( &m_dibHeader ) + sizeof( MinDibHeader ), sizeof( DibHeader ) - sizeof( MinDibHeader ) ) != 
			sizeof( DibHeader ) - sizeof( MinDibHeader ) )
		{
			return false;
		}
	}
	else
	{
		m_dibHeader.compression = NO_COMPRESSION;
		m_dibHeader.sizeImage = m_dibHeader.width * m_dibHeader.height * m_dibHeader.bitCount;
		m_dibHeader.xPelsPerMeter = 1;
		m_dibHeader.yPelsPerMeter = 1;
		m_dibHeader.clrUsed = 0;
		m_dibHeader.clrImportant = 0;
	}

	if( m_dibHeader.compression != NO_COMPRESSION )
	{
		return false;
	}
	
	m_width = m_dibHeader.width;
	m_height = m_dibHeader.height;
	m_bitsPerPixel = 32;
	m_mipLevelCount = 1;
	m_imageSize = m_width * m_height * 4;

	stream->Seek( m_bmpHeader.offBits - sizeof( DibHeader ) - sizeof( BmpHeader ), ICcpStream::SO_CURRENT );

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the format of the texture being loaded.
//   Assumes that header has been successfully read, and determined to be 
//   valid - only call after ReadHeader returns true.
// Return Value:
//   Pixel format for BMP image
// --------------------------------------------------------------------------------------
Tr2RenderContextEnum::PixelFormat Tr2BmpHandler::GetFormat() const 
{
	if( m_dibHeader.bitCount == 24 )
	{
		return Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
	}
	else
	{
		return Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM;
	}
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
bool Tr2BmpHandler::ReadImage( ICcpStream* stream )
{
	if( m_height > 4096 || m_width > 4096 ) 
	{
		CCP_LOGWARN( "Very large BMP image being loaded: %d x %d", m_width, m_height );
	}

	CCP_FREE( m_data );
	m_dataSize = GetTotalDataSize();
	m_data = (uint8_t*)CCP_MALLOC( "Tr2BmpHandler/m_data", GetTotalDataSize() );
	if( !m_data )
	{
		CCP_LOGERR( "Tr2BmpHandler couldn't allocate %d bytes (%S)", GetTotalDataSize(), m_sourceName.c_str() );
		m_dataSize = 0;
		return false;
	}

	unsigned bpp = m_dibHeader.bitCount / 8;
	unsigned stride = ( ( m_dibHeader.bitCount * m_dibHeader.width + 31 ) / 32 ) * 4;
	if( bpp != 3 )
	{
		uint8_t* data = m_data + stride * ( m_height - 1 );
		for( unsigned i = 0; i < m_height; ++i )
		{
			if( stream->Read( data, stride ) != ptrdiff_t( stride ) )
			{
				return false;
			}
			data -= stride;
		}
	}
	else
	{
		uint8_t* row = new uint8_t[stride];
		ON_BLOCK_EXIT( [&] { delete[] row; } );

		unsigned outputStride = m_width * 4;
		uint8_t* data = m_data + outputStride * ( m_height - 1 );
		for( unsigned i = 0; i < m_height; ++i )
		{
			if( stream->Read( row, stride ) != ptrdiff_t( stride ) )
			{
				return false;
			}
			unsigned inputIndex = 0;
			unsigned outputIndex = 0;
			for( unsigned j = 0; j < m_width; ++j )
			{
				data[outputIndex++] = row[inputIndex++];
				data[outputIndex++] = row[inputIndex++];
				data[outputIndex++] = row[inputIndex++];
				data[outputIndex++] = 255;
			}
			data -= outputStride;
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
unsigned Tr2BmpHandler::GetBlockByteSize() const
{
	return 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return the offset where this face and miplevel would be, relative to the start of m_data.
//   This does not include the size of the header, ie. we're dealing strictly with texels.
// Arguments:
//   mipLevel - Mip level
//   face - Cubemap face (unused: BMPs don't support cube maps)
// Return Value:
//   Offset of specified mip level.
// --------------------------------------------------------------------------------------
unsigned Tr2BmpHandler::GetOffset( unsigned mipLevel, unsigned ) const
{
	unsigned offset = 0;
	for( unsigned int i = 0; i != mipLevel ; ++i )
	{
		offset += GetMipLevelSize( i );
	}
	return offset;
}

bool Tr2BmpHandler::IsSaveSupported( const Tr2BitmapDimensions& bd )
{
	if( bd.GetType() != TEX_TYPE_2D || 
		( bd.GetFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM &&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM ) )
	{
		return false;
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves a bitmap to BMP file.
// Arguments:
//   image - Bitmap to save
//   output - Stream to save image to
// Return Value:
//   true If the image was saved
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2BmpHandler::Save( const ImageIO::HostBitmap& image, ICcpStream* output )
{
	if( !image.IsValid() )
	{
		CCP_LOGWARN( "Tr2BmpHandler::Save input image isn't valid" );
		return false;
	}

	if( !IsSaveSupported( image ) )
	{
		CCP_LOGWARN( "Tr2BmpHandler::Save does not support this image format" );
		return false;
	}

	DibHeader dibHeader;
	dibHeader.structSize = sizeof( DibHeader );
	dibHeader.width = image.GetWidth();
	dibHeader.height = image.GetHeight();
	dibHeader.planes = 1;
	dibHeader.bitCount = image.GetFormat() == PIXEL_FORMAT_B8G8R8X8_UNORM ? 24 : 32;
	dibHeader.compression = 0;

	unsigned rowSize = dibHeader.bitCount / 8 * dibHeader.width;
	unsigned padding = ( 4 - rowSize % 4 ) % 4;

	dibHeader.sizeImage = ( dibHeader.width + padding ) * dibHeader.height * dibHeader.bitCount / 8;
	dibHeader.xPelsPerMeter = 1;
	dibHeader.yPelsPerMeter = 1;
	dibHeader.clrImportant = 0;
	dibHeader.clrUsed = 0;

	BmpHeader bmpHeader;
	bmpHeader.type = BMP_SIGNATURE;
	bmpHeader.size = sizeof( BmpHeader ) + sizeof( DibHeader ) + dibHeader.sizeImage;
	bmpHeader.reserved1 = 0;
	bmpHeader.reserved2 = 0;
	bmpHeader.offBits = sizeof( BmpHeader ) + sizeof( DibHeader );

	if( output->Write( &bmpHeader, sizeof( BmpHeader ) ) != sizeof( BmpHeader ) )
	{
		CCP_LOGWARN( "Tr2BmpHandler::Save failed to write header(1)" );
		return false;
	}

	if( output->Write( &dibHeader, sizeof( DibHeader ) ) != sizeof( DibHeader ) )
	{
		CCP_LOGWARN( "Tr2BmpHandler::Save failed to write header(2)" );
		return false;
	}

	unsigned srcStride = image.GetWidth() * 4;
	const char* row = image.GetRawData() + ( image.GetHeight() - 1 ) * srcStride;

	if( dibHeader.bitCount == 32 )
	{
		for( unsigned j = 0; j < image.GetHeight(); ++j )
		{
			if( output->Write( row, srcStride ) != ptrdiff_t( srcStride ) )
			{
				CCP_LOGWARN( "Tr2BmpHandler::Save failed to write datachunk(1)" );
				return false;
			}
			if( padding )
			{
				unsigned zero = 0;
				if( output->Write( &zero, padding ) != ptrdiff_t( padding ) )
				{
					CCP_LOGWARN( "Tr2BmpHandler::Save failed to write datachunk(2)" );
					return false;
				}
			}
			row -= srcStride;
		}
	}
	else
	{
		unsigned destStride = image.GetWidth() * 3;
		char* destRow = new char[destStride];
		ON_BLOCK_EXIT( [&]{ delete[] destRow; } );
		for( unsigned j = 0; j < image.GetHeight(); ++j )
		{
			const char* current = row;
			char* dest = destRow;
			for( unsigned j = 0; j < image.GetWidth(); ++j )
			{
				*dest++ = *current++;
				*dest++ = *current++;
				*dest++ = *current++;
				current++;
			}
			if( output->Write( destRow, destStride ) != ptrdiff_t( destStride ) )
			{
				CCP_LOGWARN( "Tr2BmpHandler::Save failed to write datachunk(3)" );
				return false;
			}
			if( padding )
			{
				unsigned zero = 0;
				if( output->Write( &zero, padding ) != ptrdiff_t( padding ) )
				{
					CCP_LOGWARN( "Tr2BmpHandler::Save failed to write datachunk(4)" );
					return false;
				}
			}
			row -= srcStride;
		}
	}

	return true;
}
