// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2TgaHandler.h"
#include "HostBitmap.h"
#include "CcpMetadata.h"

using namespace ImageIO;

namespace
{

// --------------------------------------------------------------------------------------
// Description:
//   Possible values for Tr2TgaHandler::Header::colorMapType field. 
// --------------------------------------------------------------------------------------
enum ColorMapType
{
	COLOR_TYPE_RGB				= 0,
	COLOR_TYPE_INDEXED			= 1,
	__COLOR_TYPE_UNSUPPORTED,
};

// --------------------------------------------------------------------------------------
// Description:
//   Possible values for Tr2TgaHandler::Header::imageType field. 
// --------------------------------------------------------------------------------------
enum ImageType
{
	IMAGE_TYPE_RAW_INDEXED		= 1,
	IMAGE_TYPE_RAW_RGB			= 2,
	IMAGE_TYPE_RAW_GREYSCALE	= 3,
	IMAGE_TYPE_RLE_INDEXED		= 9,
	IMAGE_TYPE_RLE_RGB			= 10,
	IMAGE_TYPE_RLE_GREYSCALE	= 11,
	__IMAGE_TYPE_UNSUPPORTED,
};

// The only supported TGA palette size (256 * 3 bytes)
const unsigned PALETTE_SIZE = 768;


// ----------------------------------------------------------------------------------
// Description:
//   TGA file header structure 
// ----------------------------------------------------------------------------------
#pragma pack(push)
#pragma pack(1)
struct Header
{
	// Length of additional header data
	uint8_t idLength;
	// If file containts paletted colors
	uint8_t colorMapType;
	// Image type (pixel format and compression)
	uint8_t imageType;
	// Pallete offset
	uint16_t colorMapStart;
	// Palette length
	uint16_t colorMapLength;
	// Palette bits per pixel
	uint8_t colorMapBpp;
	// Image rectangle
	uint16_t left;
	uint16_t top;
	uint16_t right;
	uint16_t bottom;
	// Bits per pixel for image data
	uint8_t bpp;
	uint8_t imageDescriptor;
};

struct Footer
{
	uint32_t extensionOffset;
	uint32_t developerAreaOffset;
	char signature[18];
};

struct DeveloperAreaEntry
{
	uint16_t tag;
	uint32_t offset;
	uint32_t size;
};

#pragma pack(pop)

static const uint16_t CCP_DEVELOPER_TAG = ( 'C' << 8 ) | 'P';
static const char* EXPECTED_FOOTER_SIGNATURE = "TRUEVISION-XFILE.";



PixelFormat GetFormat( const Header& header ) 
{
	if( header.imageType == IMAGE_TYPE_RAW_INDEXED || header.imageType == IMAGE_TYPE_RLE_INDEXED )
	{
		return PIXEL_FORMAT_B8G8R8X8_UNORM;
	}
	else if( header.bpp == 8 )
	{
		return PIXEL_FORMAT_R8_UNORM;
	}
	else if( header.bpp < 32 )
	{
		return PIXEL_FORMAT_B8G8R8X8_UNORM;
	}
	else
	{
		return PIXEL_FORMAT_B8G8R8A8_UNORM;
	}
}

ImageIO::Result DoReadHeader( ICcpStream& stream, BitmapDimensions& dimensions, Header& header )
{
	if( stream.Read( &header, sizeof( Header ) ) != sizeof( Header ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	if( header.colorMapType >= __COLOR_TYPE_UNSUPPORTED )
	{
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}
	if( header.imageType >= __IMAGE_TYPE_UNSUPPORTED )
	{
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}
	if( header.colorMapType == COLOR_TYPE_INDEXED )
	{
		if( header.colorMapStart != 0 || header.colorMapLength != 256 || header.colorMapBpp != 24 )
		{
			return ImageIO::Result::HEADER_NOT_SUPPORTED;
		}
	}
	if( header.right - header.left < 1 || header.bottom - header.top < 1 )
	{
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}
	if( header.imageDescriptor > 63 )
	{
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}
	
	uint32_t width = unsigned( header.right - header.left );
	uint32_t height = unsigned( header.bottom - header.top );

	dimensions = BitmapDimensions( width, height, 1, GetFormat( header ) );
	return ImageIO::Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads palette data from the steam.
// Arguments:
//   stream - Data stream
//   palette - (out) Color values of palette entries
// Return Value:
//   true If the palette was loaded
//   false Otherwise
// --------------------------------------------------------------------------------------
ImageIO::Result ReadPalette( ICcpStream& stream, unsigned char*& palette )
{
	palette = new unsigned char[PALETTE_SIZE];
	if( palette == nullptr )
	{
		return ImageIO::Result::OUT_OF_MEMORY;
	}
	if( stream.Read( palette, PALETTE_SIZE ) != PALETTE_SIZE )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	return ImageIO::Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads uncompressed image data from the steam. When reading the function also 
//   performs flipping the image and converting 24bpp images to 32bpp.
// Arguments:
//   stream - Data stream
//   palette - Color values of palette entries (or null for non-indexed formats)
// Return Value:
//   true If the image was loaded
//   false Otherwise
// --------------------------------------------------------------------------------------
ImageIO::Result ReadRawData( ICcpStream& stream, const Header& header, unsigned char* palette, ImageIO::HostBitmap& bitmap )
{
	if( palette )
	{
		size_t imageSize = bitmap.GetWidth() * bitmap.GetHeight() * size_t( header.bpp / 8 );

		uint8_t *indexes = new uint8_t[imageSize];
		ON_BLOCK_EXIT( [&] { delete[] indexes; } );
		if( !indexes )
		{
			return ImageIO::Result::OUT_OF_MEMORY;
		}
		if( stream.Read( indexes, imageSize ) != ptrdiff_t( imageSize ) )
		{
			return ImageIO::Result::READ_FAILURE;
		}
		unsigned index = 0;
		unsigned stride = bitmap.GetPitch();
		uint8_t* data = reinterpret_cast<uint8_t*>( bitmap.GetRawData() ) + stride * ( bitmap.GetHeight() - 1 );
		for( unsigned j = 0; j < bitmap.GetHeight(); ++j )
		{
			unsigned rowIndex = 0;
			for( unsigned i = 0; i < bitmap.GetWidth(); ++i )
			{
				unsigned paletteIndex = indexes[index++] * 3;
				data[rowIndex++] = palette[paletteIndex];
				data[rowIndex++] = palette[paletteIndex + 1];
				data[rowIndex++] = palette[paletteIndex + 2];
				data[rowIndex++] = 255;
			}
			data -= stride;
		}
	}
	else
	{
		unsigned bpp = ( header.colorMapType == COLOR_TYPE_INDEXED ? header.colorMapBpp : header.bpp ) / 8;
		unsigned stride = bitmap.GetWidth() * bpp;
		if( bpp != 3 )
		{
			uint8_t* data = reinterpret_cast<uint8_t*>( bitmap.GetRawData() ) + stride * ( bitmap.GetHeight() - 1 );
			for( unsigned i = 0; i < bitmap.GetHeight(); ++i )
			{
				if( stream.Read( data, stride ) != ptrdiff_t( stride ) )
				{
					return ImageIO::Result::READ_FAILURE;
				}
				data -= stride;
			}
		}
		else
		{
			uint8_t* row = new uint8_t[stride];
			ON_BLOCK_EXIT( [&] { delete[] row; } );

			unsigned outputStride = bitmap.GetPitch();
			uint8_t* data = reinterpret_cast<uint8_t*>( bitmap.GetRawData() ) + outputStride * ( bitmap.GetHeight() - 1 );
			for( unsigned i = 0; i < bitmap.GetHeight(); ++i )
			{
				if( stream.Read( row, stride ) != ptrdiff_t( stride ) )
				{
					return ImageIO::Result::READ_FAILURE;
				}
				unsigned inputIndex = 0;
				unsigned outputIndex = 0;
				for( unsigned j = 0; j < bitmap.GetWidth(); ++j )
				{
					data[outputIndex++] = row[inputIndex++];
					data[outputIndex++] = row[inputIndex++];
					data[outputIndex++] = row[inputIndex++];
					data[outputIndex++] = 255;
				}
				data -= outputStride;
			}
		}
	}
	return ImageIO::Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads RLE compressed image data from the steam. When reading the function also 
//   performs flipping the image and converting 24bpp images to 32bpp.
// Arguments:
//   stream - Data stream
//   palette - Color values of palette entries (or null for non-indexed formats)
// Return Value:
//   true If the image was loaded
//   false Otherwise
// --------------------------------------------------------------------------------------
ImageIO::Result ReadRLEData( ICcpStream& stream, const Header& header, unsigned char* palette, ImageIO::HostBitmap& bitmap )
{
	size_t size = stream.GetSize() - stream.GetPosition();
	uint8_t* compressedData = new uint8_t[size];
	ON_BLOCK_EXIT( [&] { delete[] compressedData; } );
	if( !compressedData )
	{
		return ImageIO::Result::OUT_OF_MEMORY;
	}

	if( stream.Read( compressedData, size ) != ptrdiff_t( size ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}

	uint8_t* current = compressedData;
	unsigned index = 0;
	unsigned pixelSize = GetBytesPerPixel( GetFormat( header ) );
	unsigned inputPixelSize = ( header.colorMapType == COLOR_TYPE_INDEXED ? header.colorMapBpp : header.bpp ) / 8;
	unsigned outputSize = bitmap.GetMipSize( 0 );

	auto Decode = [&]( uint8_t* destination, const uint8_t* source, unsigned bpp )
	{
		if( palette )
		{
			unsigned paletteIndex = *source * 3;
			destination[0] = palette[paletteIndex];
			destination[1] = palette[paletteIndex + 1];
			destination[2] = palette[paletteIndex + 2];
			if( inputPixelSize != 4 )
			{
				destination[3] = 255;
			}
		}
		else
		{
			memcpy( destination, source, bpp );
			if( inputPixelSize != 4 )
			{
				destination[3] = 255;
			}
		}
	};

	// Decode
	while( index < outputSize ) 
	{
		if( *current & 0x80 ) // Run length chunk (High bit = 1)
		{
			int length = *current - 127;
			current++;
			if( current - compressedData > ptrdiff_t( size ) )
			{
				return ImageIO::Result::INVALID_DATA;
			}
			// Repeat the next pixel length times
			for( int loop=0; loop != length; ++loop )
			{
				Decode( reinterpret_cast<uint8_t*>( bitmap.GetRawData() ) + index, current, pixelSize );
				index += pixelSize;
			}

			current += inputPixelSize; // Move to the next descriptor chunk
			if( current - compressedData > ptrdiff_t( size ) )
			{
				return ImageIO::Result::INVALID_DATA;
			}
		}
		else // Raw chunk
		{
			int length = *current + 1;
			current++;
			if( current - compressedData > ptrdiff_t( size ) )
			{
				return ImageIO::Result::INVALID_DATA;
			}
			// Write the next length pixels directly
			for( int loop = 0; loop != length; ++loop )
			{
				Decode( reinterpret_cast<uint8_t*>( bitmap.GetRawData() ) + index, current, pixelSize );
				index += pixelSize;
				current += inputPixelSize;
				if( current - compressedData > ptrdiff_t( size ) )
				{
					return ImageIO::Result::INVALID_DATA;
				}
			}
		}
	}

	// Flip the image
	unsigned stride = bitmap.GetPitch();
	uint8_t* row0 = reinterpret_cast<uint8_t*>( bitmap.GetRawData() );
	uint8_t* row1 = reinterpret_cast<uint8_t*>( bitmap.GetRawData() ) + stride * ( bitmap.GetHeight() - 1 );
	for( unsigned i = 0; i < bitmap.GetHeight() / 2; ++i )
	{
		for( unsigned j = 0; j < stride; ++j )
		{
			std::swap( row0[j], row1[j] );
		}
		row0 += stride;
		row1 -= stride;
	}
	return ImageIO::Result::OK;
}

void SwapPixels( uint8_t* pixel0, uint8_t* pixel1, unsigned bpp )
{
	for( unsigned i = 0; i < bpp; ++i )
	{
		std::swap( *pixel0, *pixel1 );
		++pixel0;
		++pixel1;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads image data from the stream. Generates mip levels.
// Arguments:
//   stream - Data stream
// Return Value:
//   true If the image was suscessfully read
//   false If there was an error loading the image
// --------------------------------------------------------------------------------------
ImageIO::Result ReadImage( ICcpStream& stream, const Header& header, ImageIO::HostBitmap& bitmap )
{
	if( bitmap.GetWidth() > 4096 || bitmap.GetHeight() > 4096 ) 
	{
		CCP_LOGWARN( "Very large tga image being loaded: %d x %d", bitmap.GetWidth(), bitmap.GetHeight() );
	}

	unsigned char *palette = nullptr;
	ON_BLOCK_EXIT( [&] { delete[] palette; } );

	size_t fileSize = stream.GetSize();
	size_t imageSize = bitmap.GetWidth() * bitmap.GetHeight() * size_t( header.bpp / 8 );

	switch( header.imageType )
	{
	case IMAGE_TYPE_RAW_INDEXED:
		if( imageSize + sizeof( Header ) + header.idLength + PALETTE_SIZE > fileSize )
		{
			return ImageIO::Result::INVALID_DATA;
		}
		IMAGE_IO_CR_RETURN_RESULT( ReadPalette( stream, palette ) );
		IMAGE_IO_CR_RETURN_RESULT( ReadRawData( stream, header, palette, bitmap ) );
		break;
	case IMAGE_TYPE_RAW_RGB:
	case IMAGE_TYPE_RAW_GREYSCALE:
		if( imageSize + sizeof( Header ) + header.idLength > fileSize )
		{
			return ImageIO::Result::INVALID_DATA;
		}
		IMAGE_IO_CR_RETURN_RESULT( ReadRawData( stream, header, palette, bitmap ) );
		break;
	case IMAGE_TYPE_RLE_INDEXED:
		IMAGE_IO_CR_RETURN_RESULT( ReadPalette( stream, palette ) );
		IMAGE_IO_CR_RETURN_RESULT( ReadRLEData( stream, header, palette, bitmap ) );
		break;
	case IMAGE_TYPE_RLE_RGB:
	case IMAGE_TYPE_RLE_GREYSCALE:
		IMAGE_IO_CR_RETURN_RESULT( ReadRLEData( stream, header, palette, bitmap ) );
		break;
	default:
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}
	if( header.imageDescriptor & 0x10 )
	{
		// Filp horizontally
		unsigned bpp = GetBytesPerPixel( bitmap.GetFormat() );
		uint32_t width = bitmap.GetWidth();
		uint32_t height = bitmap.GetHeight();
		uint32_t pitch = bitmap.GetPitch();

		uint8_t* row = reinterpret_cast<uint8_t*>( bitmap.GetRawData() );
		for( uint32_t j = 0; j < height; ++j )
		{
			uint8_t* start = row;
			uint8_t* end = row + ( width - 1 ) * bpp;
			while( start < end )
			{
				SwapPixels( start, end, bpp );
			}
			row += pitch;
		}
	}
	if( header.imageDescriptor & 0x20 )
	{
		// Filp vertically
		unsigned bpp = GetBytesPerPixel( bitmap.GetFormat() );
		uint32_t width = bitmap.GetWidth();
		uint32_t height = bitmap.GetHeight();
		uint32_t pitch = bitmap.GetPitch();

		uint8_t* start = reinterpret_cast<uint8_t*>( bitmap.GetRawData() );
		uint8_t* end = start + ( height - 1 ) * pitch;
		while( start < end )
		{
			for( uint32_t i = 0; i < width; ++i )
			{
				SwapPixels( start + i * bpp, end + i * bpp, bpp );
			}
			start += pitch;
			end -= pitch;
		}
	}
	return ImageIO::Result::OK;
}

ImageIO::Result ReadMetadata( ICcpStream& stream, ImageIO::Metadata& metadata )
{
	if( stream.Seek( -ptrdiff_t( sizeof( Footer ) ), ICcpStream::SO_END ) < 0 )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	Footer footer;
	if( stream.Read( &footer, sizeof( footer ) ) != sizeof( footer ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	if( strncmp( footer.signature, EXPECTED_FOOTER_SIGNATURE, sizeof( footer.signature ) ) )
	{
		return ImageIO::Result::OK;
	}
	if( footer.developerAreaOffset == 0 )
	{
		return ImageIO::Result::OK;
	}
	if( stream.Seek( footer.developerAreaOffset, ICcpStream::SO_BEGIN ) < 0 )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	uint16_t count;
	if( stream.Read( &count, sizeof( count ) ) != sizeof( count ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	if( count == 0 )
	{
		return ImageIO::Result::OK;
	}
	std::unique_ptr<DeveloperAreaEntry[]> entries( new DeveloperAreaEntry[count] );
	if( stream.Read( entries.get(), sizeof( DeveloperAreaEntry ) * count ) != sizeof( DeveloperAreaEntry ) * count )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	for( uint16_t i = 0; i < count; ++i )
	{
		if( entries[i].tag == CCP_DEVELOPER_TAG )
		{
			if( stream.Seek( entries[i].offset, ICcpStream::SO_BEGIN ) < 0 )
			{
				return ImageIO::Result::READ_FAILURE;
			}
			auto result = ImageIO::LoadCcpMetadata( stream, metadata );
			if( result.code == ImageIO::Result::INVALID_HEADER )
			{
				continue;
			}
			else
			{
				return result;
			}
		}
	}
	return ImageIO::Result::OK;
}

ImageIO::Result SaveMetadata( ICcpStream& stream, const ImageIO::Metadata& metadata )
{
	if( metadata.metadata.empty() )
	{
		return ImageIO::Result::OK;
	}

	auto metadataOffset = stream.GetPosition();

	IMAGE_IO_CR_RETURN_RESULT( ImageIO::SaveCcpMetadata( stream, metadata ) );

	auto directoryOffset = stream.GetPosition();

	const uint16_t one = 1;  // one entry in the directory
	if( stream.Write( &one, sizeof( one ) ) != sizeof( one ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}

	DeveloperAreaEntry devEntry;
	devEntry.tag = CCP_DEVELOPER_TAG;
	devEntry.offset = uint32_t( metadataOffset );
	devEntry.size = uint32_t( directoryOffset - metadataOffset );

	if( stream.Write( &devEntry, sizeof( devEntry ) ) != sizeof( devEntry ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}

	Footer footer;
	strncpy_s( footer.signature, EXPECTED_FOOTER_SIGNATURE, sizeof( footer.signature ) );
	footer.extensionOffset = 0;
	footer.developerAreaOffset = uint32_t( directoryOffset );
	if( stream.Write( &footer, sizeof( footer ) ) != sizeof( footer ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	return ImageIO::Result::OK;
}

}


namespace ImageIO
{
namespace Tga
{

// --------------------------------------------------------------------------------------
// Description:
//   Registers TGA handler with ImageIO.
// --------------------------------------------------------------------------------------
void RegisterHandler()
{
	static bool s_registered = false;
	if( !s_registered )
	{
		ImageFormatFunctions funcs = { &IsTgaExtension, &ReadImage, &IsSaveSupported, &Save };
		RegisterImageHandler( funcs );
		s_registered = true;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if provided extension (without leading dot) is TGA extension.
// Arguments:
//   ext - File extension
// Return Value:
//   true If provided extension is TGA extension
// --------------------------------------------------------------------------------------
bool IsTgaExtension( const wchar_t* ext )
{
	return ( ext[0] == L't' || ext[0] == L'T' ) &&
		( ext[1] == L'g' || ext[1] == L'G' ) &&
		( ext[2] == L'a' || ext[2] == L'A' ) &&
		ext[3] == 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads TGA image from the stream.
// Arguments:
//   stream - Stream used for reading
//   loadParameters - various loading parameters
//   bitmap - (out) Destination bitmap
//   metadata - (out) Optional image metadata
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result ReadImage( ICcpStream& stream, const ImageIO::LoadParameters&, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata )
{
	if( metadata )
	{
		metadata->cutout = Cutout();
		metadata->metadata.clear();
	}

	BitmapDimensions dimensions;
	Header header;
	IMAGE_IO_CR_RETURN_RESULT( DoReadHeader( stream, dimensions, header ) );

	if( !bitmap.CreateFromBitmapDimensions( dimensions ) )
	{
		return ImageIO::Result::ERROR_CREATING_BITMAP;
	}
	auto r = ::ReadImage( stream, header, bitmap );
	if( !r )
	{
		bitmap.Destroy();
		return r;
	}

	if( metadata )
	{
		ReadMetadata( stream, *metadata );
	}
	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if saving an image into TGA format is supported.
// Arguments:
//   dimensions - Image dimensions/type/format
// Return Value:
//   Result of the operation (OK if image saving is supported)
// --------------------------------------------------------------------------------------
Result IsSaveSupported( const BitmapDimensions& bd )
{
	return bd.GetType() != TEX_TYPE_2D || bd.GetArraySize() != 1 ? Result( Result::SAVE_NOT_SUPPORTED ) : IsSaveSupported( bd.GetFormat() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if image format matches one of the formats for which saving supported.
// Arguments:
//   format - Pixel format
// Return Value:
//   Result of the operation (OK if image saving is supported)
// --------------------------------------------------------------------------------------
Result IsSaveSupported( PixelFormat format )
{
	return format == PIXEL_FORMAT_B8G8R8X8_UNORM ||
		format != PIXEL_FORMAT_B8G8R8A8_UNORM ||
		format != PIXEL_FORMAT_R8_UNORM ? Result::OK : Result::SAVE_NOT_SUPPORTED;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves TGA header to the stream.
// Arguments:
//   width - Image width
//   height - Image height
//   format - Image pixel format
//   output - Stream to save image to
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result SaveHeader( 
	uint32_t width, 
	uint32_t height, 
	PixelFormat format, 
	ICcpStream& output )
{
	IMAGE_IO_CR_RETURN_RESULT( IsSaveSupported( format ) );

	Header header;
	header.idLength = 0;
	header.colorMapType = COLOR_TYPE_RGB;
	header.colorMapStart = 0;
	header.colorMapLength = 0;
	header.colorMapBpp = 0;
	header.left = 0;
	header.top = 0;
	header.right = uint16_t( width );
	header.bottom = uint16_t( height );
	header.imageDescriptor = 0;

	switch( format )
	{
	case PIXEL_FORMAT_B8G8R8X8_UNORM:
		header.imageType = IMAGE_TYPE_RAW_RGB;
		header.bpp = 24;
		break;
	case PIXEL_FORMAT_B8G8R8A8_UNORM:
		header.imageType = IMAGE_TYPE_RAW_RGB;
		header.bpp = 32;
		break;
	case PIXEL_FORMAT_R8_UNORM:
		header.imageType = IMAGE_TYPE_RAW_GREYSCALE;
		header.bpp = 8;
		break;
    default:
		return Result::SAVE_NOT_SUPPORTED;
	}

	if( output.Write( &header, sizeof( Header ) ) != sizeof( Header ) )
	{
		return Result::WRITE_FAILURE;
	}

	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves several rows of image data into a TGA stream.
// Arguments:
//   width - Image width
//   height - Image part height
//   format - Image pixel format
//   data - Image pixel data
//   output - Stream to save image to
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result SaveRows( 
	uint32_t width, 
	uint32_t height, 
	PixelFormat format, 
	const void* data, 
	ICcpStream& output )
{
	unsigned srcBitesPerPixel;
	unsigned destBitesPerPixel;
	switch( format )
	{
	case PIXEL_FORMAT_B8G8R8X8_UNORM:
		srcBitesPerPixel = 4;
		destBitesPerPixel = 3;
		break;
	case PIXEL_FORMAT_B8G8R8A8_UNORM:
		srcBitesPerPixel = 4;
		destBitesPerPixel = 4;
		break;
	case PIXEL_FORMAT_R8_UNORM:
		srcBitesPerPixel = 1;
		destBitesPerPixel = 1;
		break;
    default:
		return Result::SAVE_NOT_SUPPORTED;
	}

	unsigned srcStride = width * srcBitesPerPixel;
	const char* row = reinterpret_cast<const char*>( data ) + ( height - 1 ) * srcStride;
	if( srcBitesPerPixel == destBitesPerPixel )
	{
		for( unsigned j = 0; j < height; ++j )
		{
			if( output.Write( row, srcStride ) != ptrdiff_t( srcStride ) )
			{
				return Result::WRITE_FAILURE;
			}
			row -= srcStride;
		}
	}
	else
	{
		CCP_ASSERT( srcBitesPerPixel == 4 && destBitesPerPixel == 3 );

		unsigned destStride = destBitesPerPixel * width;
		char* rowData = new char[destStride];
		ON_BLOCK_EXIT( [&] { delete[] rowData; } );

		for( unsigned j = 0; j < height; ++j )
		{
			const char* current = row;
			char* destRow = rowData;
			for( unsigned j = 0; j < width; ++j )
			{
				*destRow++ = *current++;
				*destRow++ = *current++;
				*destRow++ = *current++;
				current++;
			}

			if( output.Write( rowData, destStride ) != ptrdiff_t( destStride ) )
			{
				return Result::WRITE_FAILURE;
			}
			row -= srcStride;
		}
	}

	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves a bitmap to BMP file.
// Arguments:
//   image - Bitmap to save
//   output - Destination stream
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata )
{
	if( !image.IsValid() )
	{
		return Result::INVALID_BITMAP;
	}

	IMAGE_IO_CR_RETURN_RESULT( IsSaveSupported( image ) );
	IMAGE_IO_CR_RETURN_RESULT( SaveHeader( image.GetWidth(), image.GetHeight(), image.GetFormat(), output ) );
	IMAGE_IO_CR_RETURN_RESULT( SaveRows( image.GetWidth(), image.GetHeight(), image.GetFormat(), image.GetRawData(), output ) );
	if( metadata )
	{
		IMAGE_IO_CR_RETURN_RESULT( SaveMetadata( output, *metadata ) );
	}
	return Result::OK;
}

}
}