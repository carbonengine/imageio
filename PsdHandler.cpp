// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "PsdHandler.h"
#include "HostBitmap.h"

namespace
{

#pragma pack(push)
#pragma pack(1)
struct Header
{
    uint32_t signature;
    uint16_t version;
    uint8_t reserved[6];
    uint16_t channelCount;
    uint32_t height;
    uint32_t width;
    uint16_t depth;
    uint16_t colorMode;
};
#pragma pack(pop)

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

ImageIO::Result WriteZeroLengthBlock( ICcpStream& stream )
{
	uint32_t tmp = 0;
	if( stream.Write( &tmp, sizeof( tmp ) ) != sizeof( tmp ) )
	{
		return ImageIO::Result::WRITE_FAILURE;
	}
	return ImageIO::Result::OK;
}

ImageIO::PixelFormat GetFormat( Header& header ) 
{
	switch( header.channelCount )
	{
	case 1:
		return ImageIO::PIXEL_FORMAT_R8_UNORM;
	case 2:
		return ImageIO::PIXEL_FORMAT_R8G8_UNORM;
	case 3:
		return ImageIO::PIXEL_FORMAT_B8G8R8X8_UNORM;
	default:
		return ImageIO::PIXEL_FORMAT_B8G8R8A8_UNORM;
	}
}

ImageIO::Result DoReadHeader( ICcpStream& stream, ImageIO::BitmapDimensions& dimensions, Header& header, uint16_t& compression )
{
	if( stream.Read( &header, sizeof( Header ) ) != sizeof( Header ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	header.width = SwitchEndian( header.width );
	header.height = SwitchEndian( header.height );
	header.version = SwitchEndian( header.version );
	header.channelCount = SwitchEndian( header.channelCount );
	header.depth = SwitchEndian( header.depth );
	header.colorMode = SwitchEndian( header.colorMode );

	if( header.signature != PSD_SIGNATURE )
	{
		return ImageIO::Result::INVALID_HEADER;
	}

	if( header.version != 1 || header.channelCount > 4 || 
		header.depth != 8 || ( header.colorMode != PSD_COLOR_MODE_RGB && header.colorMode != PSD_COLOR_MODE_GRAYSCALE ) )
	{
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}

	SkipBlock( &stream );
	SkipBlock( &stream );
	SkipBlock( &stream );

    // Find out if the data is compressed.
    // Known values:
    //   0: no compression
    //   1: RLE compressed
 	if( stream.Read( &compression, sizeof( compression ) ) != sizeof( compression ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	compression = SwitchEndian( compression );
    if( compression > 1 )
	{
        // Unknown compression type.
        return ImageIO::Result::HEADER_NOT_SUPPORTED;
    }
	dimensions = ImageIO::BitmapDimensions( header.width, header.height, 1, GetFormat( header ) );

	return ImageIO::Result::OK;
}

ImageIO::Result ReadRleData( ICcpStream& stream, const Header& header, ImageIO::HostBitmap& bitmap )
{
	const uint32_t componentsBgra[] = { 2, 1, 0, 3 };
	const uint32_t componentsR[] = { 0, };
	const uint32_t* components = header.colorMode == PSD_COLOR_MODE_GRAYSCALE ? componentsR : componentsBgra;
	const uint32_t bpp = header.colorMode == PSD_COLOR_MODE_GRAYSCALE ? 1 : 4;
	size_t channelSize = header.width * header.height;

	stream.Seek( header.height * header.channelCount * sizeof( uint16_t ), ICcpStream::SO_CURRENT );
	CcpMallocBuffer compressedData( "PsdHandler::ReadImage/compressedData", stream.GetSize() - stream.GetPosition() );
	if( stream.Read( compressedData.get(), compressedData.size() ) != compressedData.size() )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	size_t compressedIndex = 0;

    // Read RLE data.
    for( uint32_t channel = 0; channel < header.channelCount; channel++ )
    {
		uint8_t* ptr = reinterpret_cast<uint8_t*>( bitmap.GetRawData() ) + components[channel];

        uint32_t count = 0;
        while( count < channelSize )
        {
			if( compressedIndex >= compressedData.size() )
			{
				return ImageIO::Result::INVALID_DATA;
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
					return ImageIO::Result::INVALID_DATA;
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
					return ImageIO::Result::INVALID_DATA;
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
	return ImageIO::Result::OK;
}

ImageIO::Result ReadUncompressedData( ICcpStream& stream, const Header& header, ImageIO::HostBitmap& bitmap )
{
	const uint32_t componentsBgra[] = { 2, 1, 0, 3 };
	const uint32_t componentsR[] = { 0, 1, };
	const uint32_t* components = header.channelCount < 3 ? componentsR : componentsBgra;
	const uint32_t bpp = header.channelCount == 3 ? 4 : header.channelCount;
	size_t channelSize = header.width * header.height;

	CcpMallocBuffer channel( "PsdHandler::ReadImage/channel", channelSize );
	for( uint32_t component = 0; component < header.channelCount; ++component )
	{
		if( stream.Read( channel.get(), channelSize ) != ptrdiff_t( channelSize ) )
		{
			return ImageIO::Result::READ_FAILURE;
		}
		for( size_t i = 0; i < channelSize; ++i )
		{
			reinterpret_cast<uint8_t*>( bitmap.GetRawData() )[i * bpp + components[component]] = reinterpret_cast<const uint8_t*>( channel.get() )[i];
		}
	}
	return ImageIO::Result::OK;
}

ImageIO::Result ReadImagePixels( ICcpStream& stream, ImageIO::HostBitmap& bitmap, const Header& header, uint16_t compression )
{
	if( compression == 1 )
	{
		IMAGE_IO_CR_RETURN_RESULT( ReadRleData( stream, header, bitmap ) );
	}
	else
	{
		IMAGE_IO_CR_RETURN_RESULT( ReadUncompressedData( stream, header, bitmap ) );
	}
	if( header.channelCount == 3 )
	{
		for( size_t i = 0; i < bitmap.GetWidth() * bitmap.GetHeight(); ++i )
		{
			reinterpret_cast<uint8_t*>( bitmap.GetRawData() )[i * 4 + 3] = 0xff;
		}
	}

	return ImageIO::Result::OK;
}

}

namespace ImageIO
{

namespace Psd
{

// --------------------------------------------------------------------------------------
// Description:
//   Registers PSD handler with ImageIO.
// --------------------------------------------------------------------------------------
void RegisterHandler()
{
	static bool s_registered = false;
	if( !s_registered )
	{
		ImageFormatFunctions funcs = { &IsPsdExtension, &ReadImage, &IsSaveSupported, &Save };
		RegisterImageHandler( funcs );
		s_registered = true;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if provided extension (without leading dot) is PSD extension.
// Arguments:
//   ext - File extension
// Return Value:
//   true If provided extension is PSD extension
// --------------------------------------------------------------------------------------
bool IsPsdExtension( const wchar_t* ext )
{
	return ( ext[0] == L'p' || ext[0] == L'P' ) &&
		( ext[1] == L's' || ext[1] == L'S' ) &&
		( ext[2] == L'd' || ext[2] == L'D' ) &&
		ext[3] == 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads PSD image from the stream.
// Arguments:
//   stream - Stream used for reading
//   loadParameters - various loading parameters
//   bitmap - (out) Destination bitmap
//   metadata - (out) Optional image metadata
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result ReadImage( ICcpStream& stream, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata )
{
	BitmapDimensions dimensions;
	Header header;
	uint16_t compression = 0;
	IMAGE_IO_CR_RETURN_RESULT( DoReadHeader( stream, dimensions, header, compression ) );

	if( metadata )
	{
		metadata->cutout = Cutout();
	}

	if( !bitmap.Create( header.width, header.height, 1, GetFormat( header ) ) )
	{
		return Result::ERROR_CREATING_BITMAP;
	}
	auto r = ReadImagePixels( stream, bitmap, header, compression );
	if( !r )
	{
		bitmap.Destroy();
		return r;
	}
	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if saving an image into PSD format is supported.
// Arguments:
//   dimensions - Image dimensions/type/format
// Return Value:
//   Result of the operation (OK if image saving is supported)
// --------------------------------------------------------------------------------------
Result IsSaveSupported( const BitmapDimensions& bd )
{
	if( bd.GetType() != TEX_TYPE_2D || bd.GetArraySize() != 1 ||
		( bd.GetFormat() != PIXEL_FORMAT_R8_UNORM &&
		bd.GetFormat() != PIXEL_FORMAT_R8G8_UNORM &&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM &&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM ) )
	{
		return Result::SAVE_NOT_SUPPORTED;
	}

	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves a bitmap to PSD file.
// Arguments:
//   image - Bitmap to save
//   output - Destination stream
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* )
{
	if( !image.IsValid() )
	{
		return Result::INVALID_BITMAP;
	}

	if( !IsSaveSupported( image ) )
	{
		return Result::SAVE_NOT_SUPPORTED;
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

	if( output.Write( &header, sizeof( Header ) ) != sizeof( Header ) )
	{
		return Result::WRITE_FAILURE;
	}

	IMAGE_IO_CR_RETURN_RESULT( WriteZeroLengthBlock( output ) );
	IMAGE_IO_CR_RETURN_RESULT( WriteZeroLengthBlock( output ) );
	IMAGE_IO_CR_RETURN_RESULT( WriteZeroLengthBlock( output ) );

	uint16_t compression = 0;
	if( output.Write( &compression, sizeof( compression ) ) != sizeof( compression ) )
	{
		return Result::WRITE_FAILURE;
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
		if( output.Write( channel.get(), channelSize ) != channelSize )
		{
			return Result::WRITE_FAILURE;
		}
	}
	return Result::OK;
}

}

}
