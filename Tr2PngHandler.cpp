// Copyright © 2014 CCP ehf.

#include "StdAfx.h"

#include "Tr2PngHandler.h"
#include "HostBitmap.h"
#include "png.h"

using namespace ImageIO;

namespace
{

class PngCallbacks
{
public:
	enum ReadWrite
	{
		READ,
		WRITE,
	};
	PngCallbacks( ReadWrite readWrite, png_structp png, ICcpStream& stream )
		:m_png( png ),
		m_stream( stream ),
		m_hasIoErrors( false )
	{
		if( readWrite == READ )
		{
			png_set_read_fn( png, this, &ReadData );
		}
		else
		{
			png_set_write_fn( png, this, &WriteData, &FlushData );
		}
		png_set_error_fn( png, this, &UserError, &UserWarning );
	}
	~PngCallbacks()
	{
		png_set_read_fn( m_png, nullptr, nullptr );
		png_set_error_fn( m_png, nullptr, nullptr, nullptr );
	}

	jmp_buf& GetJumpBuffer()
	{
		return m_jmpBuf;
	}

	bool IoFailed() const
	{
		return m_hasIoErrors;
	}

private:
	static void UserError( png_structp pngPtr, png_const_charp errorMsg)
	{
		if( !pngPtr || !png_get_io_ptr( pngPtr ) )
		{
			return;
		}

		PngCallbacks* self = static_cast<PngCallbacks*>( png_get_error_ptr( pngPtr ) );
		longjmp( self->m_jmpBuf, 1 );
	}

	static void UserWarning( png_structp pngPtr, png_const_charp warningMsg)
	{
	}

	static void ReadData(png_structp pngPtr, png_bytep data, png_size_t length)
	{
		if( !pngPtr || !png_get_io_ptr( pngPtr ) )
		{
			return;
		}

		PngCallbacks* self = static_cast<PngCallbacks*>( png_get_error_ptr( pngPtr ) );
		self->m_hasIoErrors = ( self->m_stream.Read( data, length ) == -1 );
	}

	static void WriteData( png_structp png, png_bytep data, png_size_t length )
	{
		PngCallbacks* self = static_cast<PngCallbacks*>( png_get_error_ptr( png ) );
		self->m_hasIoErrors = ( self->m_stream.Write( data, length ) == -1 );
	}

	static void FlushData( png_structp )
	{
	}

	png_structp m_png;
	ICcpStream& m_stream;
	bool m_hasIoErrors;
	jmp_buf m_jmpBuf;
};

png_voidp MemoryAlloc( png_structp, png_alloc_size_t size )
{
	return CCP_MALLOC( "Tr2PngHandler", size );
}
	
void MemoryFree( png_structp, png_voidp p )
{
	return CCP_FREE( p );
}

bool DoReadHeader( PngCallbacks& cbs, png_structp png, png_infop info )
{
	if( setjmp( cbs.GetJumpBuffer() ) )
	{
		return false;
	}

    png_read_info( png, info );
	return true;
}

PixelFormat GetFormat( uint32_t channels, uint32_t bitsPerPixel )
{
	switch( channels )
	{
	case 1:
		switch( bitsPerPixel )
		{
		case 8:
			return PIXEL_FORMAT_R8_UNORM;
		case 16:
			return PIXEL_FORMAT_R16_UNORM;
		}
	case 2:
		switch( bitsPerPixel )
		{
		case 16:
			return PIXEL_FORMAT_R8G8_UNORM;
		case 32:
			return PIXEL_FORMAT_R16G16_UNORM;
		}
	case 3:
		switch( bitsPerPixel )
		{
		case 24:
		case 32:
			return PIXEL_FORMAT_B8G8R8X8_UNORM;
		case 48:
			return PIXEL_FORMAT_R16G16B16A16_UNORM;
		}
	}
	switch( bitsPerPixel )
	{
	case 32:
		return PIXEL_FORMAT_B8G8R8A8_UNORM;
	case 64:
		return PIXEL_FORMAT_R16G16B16A16_UNORM;
	}
	if( channels == 3 )
	{
		return PIXEL_FORMAT_B8G8R8X8_UNORM;
	}
	else
	{
		return PIXEL_FORMAT_B8G8R8A8_UNORM;
	}
}

ImageIO::Result DoReadHeader( ICcpStream& stream, const ImageIO::LoadParameters& loadParameters, BitmapDimensions& dimensions, ImageIO::Metadata* metadata, png_structp png, png_infop info, uint32_t& channels, uint32_t& bitsPerPixel )
{
	png_byte pngsig[ 8 ];
	if( stream.Read( pngsig, sizeof( pngsig ) ) == -1 )
	{
		return ImageIO::Result::READ_FAILURE;
	}

	if( png_sig_cmp( pngsig, 0, sizeof( pngsig) ) )
	{
		return ImageIO::Result::INVALID_HEADER;
	}

	PngCallbacks cbs( PngCallbacks::READ, png, stream );

	png_set_sig_bytes( png, 8 );

	if( !DoReadHeader( cbs, png, info ) || cbs.IoFailed() )
	{
		return ImageIO::Result::INVALID_HEADER;
	}

	// ask libpng to convert on the fly to something we like
	bool isPalette     = false;
	bool isAlphaExpand = false;
	uint32_t colorType = png_get_color_type( png, info );
	if( colorType == PNG_COLOR_TYPE_PALETTE )
	{
		png_set_palette_to_rgb ( png );
		isPalette = true;
	}

	// if the image has a transperancy set.. convert it to a full Alpha channel..
	if( png_get_valid( png, info, PNG_INFO_tRNS ) )
	{
		isAlphaExpand = true;
		png_set_tRNS_to_alpha( png );
	}
    
	channels = png_get_channels( png, info );

	if( !isPalette && ( channels < 1 || channels > 4 ) )
	{
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}

	uint32_t width = png_get_image_width( png, info );
	uint32_t height = png_get_image_height( png, info );

	if( isPalette )
	{
		// auto expanded palette, possibly also alpha
		channels     = isAlphaExpand ? 4 : 3;
		bitsPerPixel = channels * 8;
	}
	else
	{
		bitsPerPixel = png_get_bit_depth( png, info ) * channels;
	}
	if( png_get_bit_depth( png, info ) > 8 )
	{
		png_set_swap( png );
	}

	if( metadata )
	{
		metadata->cutout = ImageIO::Cutout();

		const float pngUnitScale = 1.0f / 1000000.0f;
		png_int_32 x, y, unit;
		if( png_get_oFFs( png, info, &x, &y, &unit ) == PNG_INFO_oFFs && x >= 0 && y >= 0)
		{
			metadata->cutout.x = x * pngUnitScale;
			metadata->cutout.y = y * pngUnitScale;
		}
		png_uint_32 resx, resy;
		if( png_get_pHYs( png, info, &resx, &resy, &unit ) == PNG_INFO_pHYs )
		{
			metadata->cutout.width  = resx * pngUnitScale;
			metadata->cutout.height = resy * pngUnitScale;
		}

		metadata->metadata.clear();

		int numText = 0;
		png_text* text;
		png_get_text( png, info, &text, &numText );

		for( int i = 0; i < numText; ++i )
		{
			metadata->metadata.push_back( std::make_pair( text[i].key, text[i].text ) );
		}
	}
	auto format = GetFormat( channels, bitsPerPixel );
	if( format == PIXEL_FORMAT_UNKNOWN )
	{
		return ImageIO::Result::HEADER_NOT_SUPPORTED;
	}
	dimensions = BitmapDimensions( width, height, 1, format );

	return ImageIO::Result::OK;
}

bool DoReadImage( PngCallbacks& cbs, png_bytep* rowPtrs, png_structp png )
{
	if( setjmp( cbs.GetJumpBuffer() ) )
	{
		return false;
	}

	png_read_image( png, rowPtrs );
	return true;
}

ImageIO::Result ReadImagePixels( ICcpStream& stream, const ImageIO::LoadParameters& loadParameters, png_structp png, uint32_t channels, uint32_t bitsPerPixel, ImageIO::HostBitmap& bitmap )
{
	const size_t stride = ( bitmap.GetWidth() * bitsPerPixel + 7 ) / 8;
	const size_t size   = stride * bitmap.GetHeight();

	PngCallbacks cbs( PngCallbacks::READ, png, stream );

	if( bitsPerPixel == 1 && channels == 1 )
	{
		uint8_t* data = (uint8_t*)CCP_MALLOC( "Tr2PngHandler/m_data", size );
		if( !data )
		{
			return ImageIO::Result( ImageIO::Result::OUT_OF_MEMORY, "couldn't allocate " CCP_SIZET_FORMAT " bytes", size );
		}
		png_bytep* rowPtrs = CCP_NEW( "Tr2PngHandler/rowPtrs" ) png_bytep[bitmap.GetHeight()];
		for( unsigned i = 0; i != bitmap.GetHeight(); ++i )
		{
			rowPtrs[i] = data + i * stride;
		}

		if( !DoReadImage( cbs, rowPtrs, png ) || cbs.IoFailed() )
		{
			CCP_FREE( data );
			CCP_DELETE[] rowPtrs;
			return ImageIO::Result::INVALID_DATA;
		}

		CCP_DELETE[] rowPtrs;

		auto outData = reinterpret_cast<uint8_t*>( bitmap.GetRawData() );
		for( uint32_t j = 0; j < bitmap.GetHeight(); ++j )
		{
			auto inRow = data + j * stride;

			for( uint32_t i = 0; i < bitmap.GetWidth(); ++i )
			{
				uint8_t bit = ( ( 1 << ( 7 - ( i & 7 ) ) ) & inRow[i >> 3] ) ? 0xFF : 0;
				*outData++ = bit;
				*outData++ = bit;
				*outData++ = bit;
				*outData++ = 0xFF;
			}
		}
		CCP_FREE( data );
	}
	else if( channels == 3 )
	{
		uint8_t* data = (uint8_t*)CCP_MALLOC( "Tr2PngHandler/m_data", size );
		if( !data )
		{
			return ImageIO::Result( ImageIO::Result::OUT_OF_MEMORY, "couldn't allocate " CCP_SIZET_FORMAT " bytes", size );
		}

		png_bytep* rowPtrs = CCP_NEW( "Tr2PngHandler/rowPtrs") png_bytep[ bitmap.GetHeight() ];
		for( unsigned i = 0; i != bitmap.GetHeight(); ++i )
		{
			rowPtrs[i] = data + i * stride;
		}

		if( !DoReadImage( cbs, rowPtrs, png ) || cbs.IoFailed() )
		{
			CCP_FREE( data );
			CCP_DELETE [] rowPtrs;
			return ImageIO::Result::INVALID_DATA;
		}

		CCP_DELETE [] rowPtrs;

		if( bitsPerPixel == 48 )
		{
			// upsample from RGB16 to R16G16B16A16
			uint16_t* in = reinterpret_cast<uint16_t*>( data );
			uint16_t* out = reinterpret_cast<uint16_t*>( bitmap.GetRawData() );
			unsigned count = bitmap.GetWidth() * bitmap.GetHeight();
			while( count-- )
			{
				*out++ = *in++;
				*out++ = *in++;
				*out++ = *in++;
				*out++ = 0xFFFF;
			}
		}
		else
		{
			// upsample from RGB to D3DFMT_X8R8G8B8
			uint8_t* in = data;
			uint8_t* out = reinterpret_cast<uint8_t*>( bitmap.GetRawData() );
			unsigned count = bitmap.GetWidth() * bitmap.GetHeight();
			while( count-- )
			{
				*out++ = in[2];
				*out++ = in[1];
				*out++ = in[0];
				*out++ = 0xFF;
				in += 3;
			}
		}
		CCP_FREE( data );
	}
	else
	{
		png_bytep* rowPtrs = CCP_NEW( "Tr2PngHandler/rowPtrs") png_bytep[ bitmap.GetHeight() ];
		for( unsigned i = 0; i != bitmap.GetHeight(); ++i )
		{
			rowPtrs[i] = reinterpret_cast<png_bytep>( bitmap.GetRawData() ) + i * stride;
		}

		if( !DoReadImage( cbs, rowPtrs, png ) || cbs.IoFailed() )
		{
			CCP_DELETE [] rowPtrs;
			return ImageIO::Result::INVALID_DATA;
		}

		CCP_DELETE [] rowPtrs;

		if( channels > 2 && bitsPerPixel == 32 )
		{
			const unsigned bytes = bitsPerPixel / 8;

			for( unsigned j = 0; j != bitmap.GetHeight(); ++j )
			{
				char* RGBA = bitmap.GetRawData() + j * stride;
				for( unsigned px = 0; px != bitmap.GetWidth(); ++px, RGBA += bytes )
				{
					std::swap( RGBA[0], RGBA[2] );
				}
			}
		}
	}

	return ImageIO::Result::OK;
}

bool DoSaveImage( PngCallbacks& cbs, png_structp png, png_infop info, int transforms )
{
	if( setjmp( cbs.GetJumpBuffer() ) )
	{
		return false;
	}
	png_write_png( png, info, transforms, nullptr );
	png_write_info( png, info );
	return true;
}


}


namespace ImageIO
{
namespace Png
{

// --------------------------------------------------------------------------------------
// Description:
//   Registers PNG handler with ImageIO.
// --------------------------------------------------------------------------------------
void RegisterHandler()
{
	static bool s_registered = false;
	if( !s_registered )
	{
		ImageFormatFunctions funcs = { &IsPngExtension, &ReadImage, &IsSaveSupported, &Save };
		RegisterImageHandler( funcs );
		s_registered = true;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if provided extension (without leading dot) is PNG extension.
// Arguments:
//   ext - File extension
// Return Value:
//   true If provided extension is BMP extension
// --------------------------------------------------------------------------------------
bool IsPngExtension( const wchar_t* ext )
{
	return ( ext[0] == L'p' || ext[0] == L'P' ) &&
		( ext[1] == L'n' || ext[1] == L'N' ) &&
		( ext[2] == L'g' || ext[2] == L'G' ) &&
		ext[3] == 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads PNG image from the stream.
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
	png_structp png = png_create_read_struct_2( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, MemoryAlloc, MemoryFree );
	png_infop info = png_create_info_struct( png );

	if( !png || !info )
	{
		return Result::OUT_OF_MEMORY;
	}

	ON_BLOCK_EXIT( [&] { png_destroy_read_struct( &png, &info, 0 ); } );

	BitmapDimensions dimensions;
	uint32_t channels = 0, bitsPerPixel = 0;
	IMAGE_IO_CR_RETURN_RESULT( DoReadHeader( stream, loadParameters, dimensions, metadata, png, info, channels, bitsPerPixel ) );

	if( !bitmap.CreateFromBitmapDimensions( dimensions ) )
	{
		return Result::ERROR_CREATING_BITMAP;
	}

	auto r = ReadImagePixels( stream, loadParameters, png, channels, bitsPerPixel, bitmap );
	if( !r )
	{
		bitmap.Destroy();
		return r;
	}
	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if saving an image into PNG format is supported.
// Arguments:
//   dimensions - Image dimensions/type/format
// Return Value:
//   Result of the operation (OK if image saving is supported)
// --------------------------------------------------------------------------------------
Result IsSaveSupported( const BitmapDimensions& bd )
{
	if( ( bd.GetType() != TEX_TYPE_1D &&
		bd.GetType() != TEX_TYPE_2D ) ||
		bd.GetArraySize() != 1 )
	{
		return Result::SAVE_NOT_SUPPORTED;
	}

	if( bd.GetFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM		&&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM		&& 
		bd.GetFormat() != PIXEL_FORMAT_R8_UNORM				&&
		bd.GetFormat() != PIXEL_FORMAT_A8_UNORM				&&
		bd.GetFormat() != PIXEL_FORMAT_R16_UNORM			&& 
		bd.GetFormat() != PIXEL_FORMAT_R16G16B16A16_UNORM	&& 
		bd.GetFormat() != PIXEL_FORMAT_R10G10B10A2_UNORM	&&
		bd.GetFormat() != PIXEL_FORMAT_R10G10B10A2_TYPELESS )
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
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata )
{
	if( !image.IsValid() )
	{
		return Result::INVALID_BITMAP;
	}

	if( !IsSaveSupported( image ) )
	{
		return Result::SAVE_NOT_SUPPORTED;
	}

	png_structp png = png_create_write_struct_2( PNG_LIBPNG_VER_STRING, 
												 nullptr, 
												 nullptr, 
												 nullptr, 
												 nullptr, 
												 &MemoryAlloc, 
												 &MemoryFree );

	if( !png )
	{
		return Result::OUT_OF_MEMORY;
	}

	png_infop info = png_create_info_struct( png );
	if( !info )
	{
		return Result::OUT_OF_MEMORY;
	}

	Result result = Result::OK;
	{
	PngCallbacks cbs( PngCallbacks::WRITE, png, output );

	int colorType;
	int bitDepth = 8;
	int transforms = PNG_TRANSFORM_BGR;
	switch( image.GetFormat() ) 
	{
	case PIXEL_FORMAT_R8_UNORM:
	case PIXEL_FORMAT_A8_UNORM:
	case PIXEL_FORMAT_R16_UNORM:
		colorType = PNG_COLOR_TYPE_GRAY;
		break;
	case PIXEL_FORMAT_B8G8R8X8_UNORM:
	case PIXEL_FORMAT_R10G10B10A2_TYPELESS:
	case PIXEL_FORMAT_R10G10B10A2_UNORM:
		colorType = PNG_COLOR_TYPE_RGB;
		break;
	default:
		colorType = PNG_COLOR_TYPE_RGB_ALPHA;
	}
	switch( image.GetFormat() )
	{
	case PIXEL_FORMAT_R16_UNORM:
	case PIXEL_FORMAT_R16G16B16A16_UNORM:
		bitDepth = 16;
		transforms = PNG_TRANSFORM_SWAP_ENDIAN;
		break;
    default:
        break;
	}
	//PNG_COLOR_TYPE_GRAY
	png_set_IHDR( 
		png, 
		info, 
		image.GetWidth(), 
		image.GetHeight(),
		bitDepth, 
		colorType, 
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, 
		PNG_FILTER_TYPE_DEFAULT );

	const bool is1010102 = 
			image.GetFormat() == PIXEL_FORMAT_R10G10B10A2_TYPELESS ||
			image.GetFormat() == PIXEL_FORMAT_R10G10B10A2_UNORM;

	int rSwizzle = 0;
	int bSwizzle = 2;

	png_bytep* rows = CCP_NEW( "Tr2PngHandler/rows" ) png_bytep[image.GetHeight()];
	if( colorType == PNG_COLOR_TYPE_RGB )
	{
		for( unsigned i = 0; i < image.GetHeight(); ++i )
		{
			rows[i] = CCP_NEW( "Tr2PngHandler/rows[i]" ) png_byte[image.GetWidth() * 3];
			const uint8_t* srcRow = reinterpret_cast<const uint8_t*>( image.GetRawData() + i * image.GetPitch() );
			uint8_t* destRow = reinterpret_cast<uint8_t*>( rows[i] );

			if( is1010102 )
			{
				for( unsigned j = 0; j != image.GetWidth(); ++j, destRow += 3, srcRow += 4 )
				{
					const uint32_t rgb10 = *(const uint32_t*)srcRow;
					destRow[bSwizzle] = uint8_t( ( ( rgb10 >>  0 ) & 1023 ) >> 2 );
					destRow[1] = uint8_t( ( ( rgb10 >> 10 ) & 1023 ) >> 2 );
					destRow[rSwizzle] = uint8_t( ( ( rgb10 >> 20 ) & 1023 ) >> 2 );
				}
			}
			else
			{
				for( unsigned j = 0; j < image.GetWidth(); ++j )
				{
					*destRow++ = *srcRow++;
					*destRow++ = *srcRow++;
					*destRow++ = *srcRow++;
					srcRow++;
				}
			}
		}
	}
	else
	{
		for( unsigned i = 0; i < image.GetHeight(); ++i )
		{
			rows[i] = ( png_bytep )( image.GetRawData() + i * image.GetPitch() );
		}
	}

	std::unique_ptr<png_text> text;

	if( metadata )
	{
		if( !metadata->metadata.empty() )
		{
			text.reset( new png_text[metadata->metadata.size()] );
			for( size_t i = 0; i < metadata->metadata.size(); ++i )
			{
				text.get()[i].compression = PNG_TEXT_COMPRESSION_NONE;
				text.get()[i].key = (png_charp)metadata->metadata[i].first.c_str();
				text.get()[i].text = (png_charp)metadata->metadata[i].first.c_str();
				text.get()[i].lang = nullptr;
				text.get()[i].lang_key = nullptr;
				text.get()[i].itxt_length = 0;
				text.get()[i].text_length = 0;
			}
			png_set_text( png, info, text.get(), int( metadata->metadata.size() ) );
		}
	}

	png_set_rows( png, info, rows );

	if( !DoSaveImage( cbs, png, info, transforms ) )
	{
		result = Result::WRITE_FAILURE;
	}

	if( colorType == PNG_COLOR_TYPE_RGB )
	{
		for( unsigned i = 0; i < image.GetHeight(); ++i )
		{
			CCP_DELETE[] rows[i];
		}
	}
	CCP_DELETE[] rows;
	}
	png_destroy_write_struct( &png, &info );
	return result;
}

}
}
