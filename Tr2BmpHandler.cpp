// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2BmpHandler.h"
#include "HostBitmap.h"

using namespace ImageIO;

namespace
{

const unsigned short BMP_SIGNATURE = 0x4D42;
const unsigned NO_COMPRESSION = 0;

#pragma pack(push)
#pragma pack(1)

struct BmpHeader
{
	// signature - 'BM'
	uint16_t type;     
	// file size in bytes
	uint32_t size;   
	// 0
	uint16_t reserved1;     
	// 0
	uint16_t reserved2; 
	// offset to bitmap
	uint32_t offBits;  
};

struct MinDibHeader
{
	// size of this struct (40)
	uint32_t structSize;
	// bmap width in pixels
	uint32_t width; 
	// bmap height in pixels
	uint32_t height;    
	// num planes - always 1
	uint16_t planes;  
	// bits per pixel
	uint16_t bitCount;
};

struct DibHeader: public MinDibHeader
{
	// compression flag
	uint32_t compression;   
	// image size in bytes
	uint32_t sizeImage;     
	// horz resolution
	int32_t xPelsPerMeter; 
	// vert resolution
	int32_t yPelsPerMeter; 
	// 0 -> color table size
	uint32_t clrUsed;
	// important color count
	uint32_t clrImportant;  
};
#pragma pack(pop)

PixelFormat GetFormat( DibHeader& dibHeader ) 
{
	if( dibHeader.bitCount == 24 )
	{
		return PIXEL_FORMAT_B8G8R8X8_UNORM;
	}
	else
	{
		return PIXEL_FORMAT_B8G8R8A8_UNORM;
	}
}

ImageIO::Result DoReadHeader( ICcpStream& stream, BitmapDimensions& dimensions, BmpHeader& bmpHeader, DibHeader& dibHeader )
{
	if( stream.Read( &bmpHeader, sizeof( BmpHeader ) ) != sizeof( BmpHeader ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	if( bmpHeader.type != BMP_SIGNATURE )
	{
		return ImageIO::Result::INVALID_HEADER;
	}
	if( stream.Read( &dibHeader, sizeof( MinDibHeader ) ) != sizeof( MinDibHeader ) )
	{
		return ImageIO::Result::READ_FAILURE;
	}
	if( dibHeader.bitCount != 24 && dibHeader.bitCount != 32 )
	{
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}
	if( dibHeader.structSize >= sizeof( DibHeader ) )
	{
		if( stream.Read( reinterpret_cast<char*>( &dibHeader ) + sizeof( MinDibHeader ), sizeof( DibHeader ) - sizeof( MinDibHeader ) ) != 
			sizeof( DibHeader ) - sizeof( MinDibHeader ) )
		{
			return ImageIO::Result::READ_FAILURE;
		}
	}
	else
	{
		dibHeader.compression = NO_COMPRESSION;
		dibHeader.sizeImage = dibHeader.width * dibHeader.height * dibHeader.bitCount;
		dibHeader.xPelsPerMeter = 1;
		dibHeader.yPelsPerMeter = 1;
		dibHeader.clrUsed = 0;
		dibHeader.clrImportant = 0;
	}

	if( dibHeader.compression != NO_COMPRESSION )
	{
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}
	
	uint32_t width = dibHeader.width;
	uint32_t height = dibHeader.height;

	dimensions = BitmapDimensions( width, height, 1, GetFormat( dibHeader ) );
	return ImageIO::Result::OK;
}

ImageIO::Result ReadImagePixels( ICcpStream& stream, ImageIO::HostBitmap& bitmap, const DibHeader& dibHeader )
{
	unsigned bpp = dibHeader.bitCount / 8;
	unsigned stride = ( ( dibHeader.bitCount * dibHeader.width + 31 ) / 32 ) * 4;
	unsigned outputStride = bitmap.GetWidth() * 4;
	if( bpp != 3 )
	{
		uint8_t* data = reinterpret_cast<uint8_t*>( bitmap.GetRawData() ) + stride * ( bitmap.GetHeight() - 1 );
		for( unsigned i = 0; i < bitmap.GetHeight(); ++i )
		{
			if( stream.Read( data, stride ) != ptrdiff_t( stride ) )
			{
				return ImageIO::Result::READ_FAILURE;
			}
			if( stride != outputStride )
			{
				stream.Seek( stride - outputStride, ICcpStream::SO_CURRENT );
			}
			data -= stride;
		}
	}
	else
	{
		uint8_t* row = new uint8_t[stride];
		ON_BLOCK_EXIT( [&] { delete[] row; } );

		unsigned outputStride = bitmap.GetWidth() * 4;
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

	return ImageIO::Result::OK;
}

}

namespace ImageIO
{
namespace Bmp
{
	
// --------------------------------------------------------------------------------------
// Description:
//   Registers BMP handler with ImageIO.
// --------------------------------------------------------------------------------------
void RegisterHandler()
{
	static bool s_registered = false;
	if( !s_registered )
	{
		ImageFormatFunctions funcs = { &IsBmpExtension, &ReadImage, &IsSaveSupported, &Save };
		RegisterImageHandler( funcs );
		s_registered = true;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if provided extension (without leading dot) is BMP extension.
// Arguments:
//   ext - File extension
// Return Value:
//   true If provided extension is BMP extension
// --------------------------------------------------------------------------------------
bool IsBmpExtension( const wchar_t* ext )
{
	return ( ext[0] == L'b' || ext[0] == L'B' ) &&
		( ext[1] == L'm' || ext[1] == L'M' ) &&
		( ext[2] == L'p' || ext[2] == L'P' ) &&
		ext[3] == 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads BMP image from the stream.
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
	BitmapDimensions dimensions;
	BmpHeader bmpHeader;
	DibHeader dibHeader;
	IMAGE_IO_CR_RETURN_RESULT( DoReadHeader( stream, dimensions, bmpHeader, dibHeader ) );

	stream.Seek( bmpHeader.offBits - sizeof( DibHeader ) - sizeof( BmpHeader ), ICcpStream::SO_CURRENT );

	if( metadata )
	{
		metadata->cutout = Cutout();
	}
	
	if( !bitmap.CreateFromBitmapDimensions( dimensions ) )
	{
		return Result::ERROR_CREATING_BITMAP;
	}
	auto r = ReadImagePixels( stream, bitmap, dibHeader );
	if( !r )
	{
		bitmap.Destroy();
		return r;
	}
	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if saving an image into BMP format is supported.
// Arguments:
//   dimensions - Image dimensions/type/format
// Return Value:
//   Result of the operation (OK if image saving is supported)
// --------------------------------------------------------------------------------------
Result IsSaveSupported( const BitmapDimensions& bd )
{
	if( bd.GetType() != TEX_TYPE_2D || bd.GetArraySize() != 1 || 
		( bd.GetFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM &&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM ) )
	{
		return Result::SAVE_NOT_SUPPORTED;
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

	if( output.Write( &bmpHeader, sizeof( BmpHeader ) ) != sizeof( BmpHeader ) )
	{
		return Result::WRITE_FAILURE;
	}

	if( output.Write( &dibHeader, sizeof( DibHeader ) ) != sizeof( DibHeader ) )
	{
		return Result::WRITE_FAILURE;
	}

	unsigned srcStride = image.GetWidth() * 4;
	const char* row = image.GetRawData() + ( image.GetHeight() - 1 ) * srcStride;

	if( dibHeader.bitCount == 32 )
	{
		for( unsigned j = 0; j < image.GetHeight(); ++j )
		{
			if( output.Write( row, srcStride ) != ptrdiff_t( srcStride ) )
			{
				return Result::WRITE_FAILURE;
			}
			if( padding )
			{
				unsigned zero = 0;
				if( output.Write( &zero, padding ) != ptrdiff_t( padding ) )
				{
					return Result::WRITE_FAILURE;
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
			if( output.Write( destRow, destStride ) != ptrdiff_t( destStride ) )
			{
				return Result::WRITE_FAILURE;
			}
			if( padding )
			{
				unsigned zero = 0;
				if( output.Write( &zero, padding ) != ptrdiff_t( padding ) )
				{
					return Result::WRITE_FAILURE;
				}
			}
			row -= srcStride;
		}
	}

	return Result::OK;
}

}
}