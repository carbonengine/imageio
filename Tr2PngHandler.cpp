#include "StdAfx.h"

#include "Tr2PngHandler.h"
#include "HostBitmap.h"

using namespace Tr2RenderContextEnum;

extern bool g_isR10G10B10FormatInverted;

namespace
{
	struct HandlerAndStream
	{
		Tr2PngHandler* handler;
		ICcpStream* stream;
	};
}

Tr2PngHandler::Tr2PngHandler( const wchar_t* sourceName )
: Tr2ImageHandler( sourceName )
, m_png ( NULL )
, m_info( NULL )
, m_channels( 0 )
, m_haveError( false )
{}

Tr2PngHandler::~Tr2PngHandler()
{
	png_destroy_read_struct( &m_png, &m_info, 0 );	
}

bool Tr2PngHandler::IsValid() const
{ 
	return m_png != NULL && m_info != NULL && !m_haveError; 
}

png_voidp Tr2PngHandler::MemoryAlloc( png_structp, png_alloc_size_t size )
{
	return CCP_MALLOC( "Tr2PngHandler", size );
}
	
void Tr2PngHandler::MemoryFree( png_structp, png_voidp p )
{
	return CCP_FREE( p );
}

void Tr2PngHandler::ReadData( ICcpStream* stream, png_bytep data, png_size_t length )
{
	if( stream )
	{
		m_haveError = ( stream->Read( data, length ) == -1 );
	}
}

void Tr2PngHandler::ReadData_thunk(png_structp pngPtr, png_bytep data, png_size_t length)
{
	if( !pngPtr || !png_get_io_ptr( pngPtr ) )
	{
		return;
	}

	auto handlerAndStream = static_cast<HandlerAndStream*>( png_get_io_ptr( pngPtr ) );
	handlerAndStream->handler->ReadData( handlerAndStream->stream, data, length );
}

void Tr2PngHandler::UserError( png_const_charp errorMsg)
{
	CCP_LOGERR( "PNG error: %s (%S)", errorMsg ? errorMsg : "(unknown error)", m_sourceName.empty() ? L"(unknown file)" : m_sourceName.c_str() );

	m_haveError = true;
	m_width = m_height = m_bitsPerPixel = 0;

	longjmp( m_jmpBuf, 1 );
}

void Tr2PngHandler::UserWarning( png_const_charp warningMsg)
{
	CCP_LOGWARN( "PNG warning: %s (%S)", warningMsg ? warningMsg : "(unknown warning)", m_sourceName.empty() ? L"(unknown file)" : m_sourceName.c_str() );
}

void Tr2PngHandler::UserError_thunk( png_structp pngPtr, png_const_charp errorMsg)
{
	if( !pngPtr || !png_get_io_ptr( pngPtr ) )
	{
		return;
	}

	static_cast<Tr2PngHandler*>( png_get_error_ptr( pngPtr ) )->UserError( errorMsg );
}

void Tr2PngHandler::UserWarning_thunk( png_structp pngPtr, png_const_charp warningMsg)
{
	if( !pngPtr || !png_get_io_ptr( pngPtr ) )
	{
		return;
	}

	static_cast<Tr2PngHandler*>( png_get_error_ptr( pngPtr ) )->UserWarning( warningMsg );
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function that reads header information from PNG. It is a separate function so
//   that long jumps have minimal interference with C++.
// Return Value:
//   true If header was read
//   false On error
// --------------------------------------------------------------------------------------
bool Tr2PngHandler::DoReadHeader()
{
	if( setjmp( m_jmpBuf ) )
	{
		return false;
	}

    png_read_info( m_png, m_info );
	return true;
}

bool Tr2PngHandler::ReadHeader( ICcpStream* stream )
{
	png_byte pngsig[ 8 ];
	if( !stream || stream->Read( pngsig, sizeof( pngsig ) ) == -1 )
	{
		return false;
	}

	if( png_sig_cmp( pngsig, 0, sizeof( pngsig) ) )
	{
		return false;
	}

	m_png  = png_create_read_struct_2( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL, NULL, MemoryAlloc, MemoryFree );
	m_info = png_create_info_struct( m_png );

	if( !m_png || !m_info )
	{
		return false;
	}

	HandlerAndStream* handlerAndStream = CCP_NEW( "Tr2PngHandler::HandlerAndStream" ) HandlerAndStream;
	handlerAndStream->handler = this;
	handlerAndStream->stream = stream;

	png_set_read_fn ( m_png, handlerAndStream, &Tr2PngHandler::ReadData_thunk );
	png_set_error_fn( m_png, this, &Tr2PngHandler::UserError_thunk, &Tr2PngHandler::UserWarning_thunk );

	if( !m_png || !m_info )
	{
		// SetStream wasn't called, or failed.
		return false;
	}

	png_set_sig_bytes( m_png, 8 );

	if( !DoReadHeader() )
	{
		return false;
	}

	// ask libpng to convert on the fly to something we like
	bool isPalette     = false;
	bool isAlphaExpand = false;
	switch( GetColorType() )
	{
		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb ( m_png );
			isPalette = true;
			break;

		case PNG_COLOR_TYPE_GRAY:
			/*
			if( m_info->bit_depth < 8 )
			{
				png_set_expand_gray_1_2_4_to_8( m_png );
			}
			*/
			break;
	}

	// if the image has a transperancy set.. convert it to a full Alpha channel..
	if( png_get_valid( m_png, m_info, PNG_INFO_tRNS ) )
	{
		isAlphaExpand = true;
		png_set_tRNS_to_alpha( m_png );
	}
    
	m_channels = IsValid() ? png_get_channels    ( m_png, m_info ) : 0;

	if( !isPalette && ( m_channels < 1 || m_channels > 4 ) )
	{
		return false;
	}

	m_width    = IsValid() ? png_get_image_width ( m_png, m_info ) : 0;
	m_height   = IsValid() ? png_get_image_height( m_png, m_info ) : 0;
	
	if( isPalette )
	{
		// auto expanded palette, possibly also alpha
		m_channels     = isAlphaExpand ? 4 : 3;
		m_bitsPerPixel = m_channels * 8;
	}
	else
	{
		m_bitsPerPixel = IsValid() ? png_get_bit_depth( m_png, m_info ) * m_channels : 0;
	}
	if( png_get_bit_depth( m_png, m_info ) > 8 )
	{
		png_set_swap( m_png );
	}

	const float pngUnitScale = 1.0f / 1000000.0f;
	png_int_32 x, y, unit;
	if( png_get_oFFs( m_png, m_info, &x, &y, &unit ) == PNG_INFO_oFFs && x >= 0 && y >= 0)
	{
		m_cutoutX = x * pngUnitScale;
		m_cutoutY = y * pngUnitScale;
	}
	png_uint_32 resx, resy;
	if( png_get_pHYs( m_png, m_info, &resx, &resy, &unit ) == PNG_INFO_pHYs )
	{
		m_cutoutWidth  = resx * pngUnitScale;
		m_cutoutHeight = resy * pngUnitScale;
	}

	return !m_haveError;
}

bool Tr2PngHandler::IsSupported() const
{
	if(	m_bitsPerPixel == 8  ||
		m_bitsPerPixel == 16 ||
		m_bitsPerPixel == 24 ||
		m_bitsPerPixel == 32 ||
		m_bitsPerPixel == 48 ||
		m_bitsPerPixel == 64 )
	{
		return true;
	}

	CCP_LOGWARN(	"Tr2PngHandler: '%S' has unsupported bitsPerPixel %d", 
					m_sourceName.c_str(), 
					m_bitsPerPixel );

	return false;
}

Tr2RenderContextEnum::PixelFormat Tr2PngHandler::GetFormat() const
{
	switch( m_channels )
	{
	case 1:
		switch( m_bitsPerPixel )
		{
		case 8:
			return PIXEL_FORMAT_R8_UNORM;
		case 16:
			return PIXEL_FORMAT_R16_UNORM;
		}
	case 2:
		switch( m_bitsPerPixel )
		{
		case 16:
			return PIXEL_FORMAT_R8G8_UNORM;
		case 32:
			return PIXEL_FORMAT_R16G16_UNORM;
		}
	case 3:
		switch( m_bitsPerPixel )
		{
		case 24:
		case 32:
			return PIXEL_FORMAT_B8G8R8X8_UNORM;
		case 48:
			return PIXEL_FORMAT_R16G16B16A16_UNORM;
		}
	}
	switch( m_bitsPerPixel )
	{
	case 32:
		return PIXEL_FORMAT_B8G8R8A8_UNORM;
	case 64:
		return PIXEL_FORMAT_R16G16B16A16_UNORM;
	}
	if( m_channels == 3 )
	{
		return PIXEL_FORMAT_B8G8R8X8_UNORM;
	}
	else
	{
		return PIXEL_FORMAT_B8G8R8A8_UNORM;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function that reads image pixels from PNG. It is a separate function so
//   that long jumps have minimal interference with C++.
// Arguments:
//   rowPtrs - array of pointers to rows of pixels to store image data
// Return Value:
//   true If image was read
//   false On error
// --------------------------------------------------------------------------------------
bool Tr2PngHandler::DoReadImage( png_bytep* rowPtrs )
{
	if( setjmp( m_jmpBuf ) )
	{
		return false;
	}

	png_read_image( m_png, rowPtrs );
	return true;
}

bool Tr2PngHandler::ReadImage( ICcpStream* stream )
{
	if( !IsValid() || !IsSupported() )
	{
		return false;
	}

	const size_t stride = GetWidth() * m_bitsPerPixel / 8;
	const size_t size   = stride * GetHeight();

	m_data = (uint8_t*)CCP_MALLOC( "Tr2PngHandler/m_data", size );
	if( !m_data )
    {
		CCP_LOGERR( "TriPNGHandler couldn't allocate %d bytes (%S)", size, m_sourceName.c_str() );
        return false;
    }

	png_bytep* rowPtrs = CCP_NEW( "Tr2PngHandler/rowPtrs") png_bytep[ GetHeight() ];
	for( unsigned i = 0; i != GetHeight(); ++i )
	{
		rowPtrs[i] = m_data + i * stride;
	}

	if( !DoReadImage( rowPtrs ) )
	{
		CCP_DELETE [] rowPtrs;
		return false;
	}

	CCP_DELETE [] rowPtrs;

	if( !m_haveError && m_channels == 3 )
	{
		// upsample from RGB to D3DFMT_X8R8G8B8
		// upsample from RGB to D3DFMT_X8R8G8B8
		if( m_bitsPerPixel == 48 )
		{
			unsigned newDataSize = GetWidth() * GetHeight() * 8;
			uint8_t* newData = (uint8_t*)CCP_MALLOC( "Tr2PngHandler/newData", newDataSize );

			if( !newData )
			{
				CCP_LOGERR( "Tr2PngHandler/newData - out of memory!" );
				m_haveError = true;

				CCP_FREE( m_data );

				m_dataSize		= 0;
				m_data			= NULL;
				m_bitsPerPixel	= 0;
				m_channels		= 0;
			}
			else
			{
				uint16_t* in = reinterpret_cast<uint16_t*>( m_data );
				uint16_t* out = reinterpret_cast<uint16_t*>( newData );
				unsigned count = GetWidth() * GetHeight();
				while( count-- )
				{
					*out++ = *in++;
					*out++ = *in++;
					*out++ = *in++;
					*out++ = 0xFFFF;
				}
				CCP_FREE( m_data );

				m_dataSize = newDataSize;
				m_data = newData;
				m_bitsPerPixel = 64;
				m_channels = 4;
			}
		}
		else
		{
			unsigned newDataSize = GetWidth() * GetHeight() * 4;
			uint8_t* newData = (uint8_t*)CCP_MALLOC( "Tr2PngHandler/newData", newDataSize );

			if( !newData )
			{
				CCP_LOGERR( "Tr2PngHandler/newData - out of memory!" );
				m_haveError = true;

				CCP_FREE( m_data );

				m_dataSize		= 0;
				m_data			= NULL;
				m_bitsPerPixel	= 0;
				m_channels		= 0;
			}
			else
			{
				uint8_t* in = m_data;
				uint8_t* out = newData;
				unsigned count = GetWidth() * GetHeight();
				while( count-- )
				{
					*out++ = in[2];
					*out++ = in[1];
					*out++ = in[0];
					*out++ = 0xFF;
					in += 3;
				}
				CCP_FREE( m_data );

				m_dataSize		= newDataSize;
				m_data			= newData;
				m_bitsPerPixel	= 32;
			}
		}
	}
	else if( !m_haveError && m_channels > 2 && m_bitsPerPixel == 32 )
	{
		const unsigned bytes = m_bitsPerPixel / 8;

		for( unsigned j = 0; j != GetHeight(); ++j )
		{
			unsigned char* RGBA = m_data + j * stride;
			for( unsigned px = 0; px != GetWidth(); ++px, RGBA += bytes )
			{
				std::swap( RGBA[0], RGBA[2] );
			}
		}
	}
        


	if( m_haveError )
	{
		return false;
	}

	if( !ConvertDesiredFormat() )
	{
		return false;
	}

	return true;
}

unsigned int Tr2PngHandler::GetBlockByteSize() const
{
	// Not a block image.
	return 0;
}

unsigned Tr2PngHandler::GetOffset( unsigned mipLevel, unsigned face ) const
{
	CCP_ASSERT( mipLevel == 0 && face == 0 );
	return 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Examines bitmap information to see if the image can be saved to PNG format. 
//   We only support 1D/2D images with pixel formats B8G8R8A8, B8G8R8X8 or R8.
// Arguments:
//   bd - Bitmap information
// Return Value:
//   true If saving of such image is supported
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2PngHandler::IsSaveSupported( const Tr2BitmapDimensions& bd )
{
	if( bd.GetType() != Tr2RenderContextEnum::TEX_TYPE_1D &&
		bd.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
	{
		CCP_LOGWARN( "Tr2PngHandler::Save does not support this texture type: %d", bd.GetType() );
		return false;
	}

	if( bd.GetFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM		&&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM		&& 
		bd.GetFormat() != PIXEL_FORMAT_R8_UNORM				&&
		bd.GetFormat() != PIXEL_FORMAT_R16_UNORM			&& 
		bd.GetFormat() != PIXEL_FORMAT_R16G16B16A16_UNORM	&& 
		bd.GetFormat() != PIXEL_FORMAT_R10G10B10A2_UNORM	&&
		bd.GetFormat() != PIXEL_FORMAT_R10G10B10A2_TYPELESS )
	{
		CCP_LOGWARN( "Tr2JpgHandler::Save does not support this image format: %d", bd.GetFormat() );
		return false;
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function to write thunk of PNG encoded data to file.
// Arguments:
//   png - PNG object
//   data - Pointer to data to write
//   length - Size of data
// See Also:
//   Tr2PngHandler::Save
// --------------------------------------------------------------------------------------
static void WriteDataThunk( png_structp png, png_bytep data, png_size_t length )
{
	ICcpStream* output = reinterpret_cast<ICcpStream*>( png_get_io_ptr( png ) );
	output->Write( data, length );
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function that is supposed to flush written file. Does nothing.
// Arguments:
//   png - PNG object
// See Also:
//   Tr2PngHandler::Save
// --------------------------------------------------------------------------------------
static void FlushData( png_structp )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function that saves image to PNG. It is a separate function so
//   that long jumps have minimal interference with C++.
// Arguments:
//   png - PNG object
//   info - PNG info object
// Return Value:
//   true If image was saved
//   false On error
// --------------------------------------------------------------------------------------
bool Tr2PngHandler::DoSaveImage( png_structp png, png_infop info, int transforms )
{
	if( setjmp( png_jmpbuf( png ) ) )
	{
		return false;
	}
	png_write_png( png, info, transforms, nullptr );
	png_write_info( png, info );
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves provided bitmap as a PNG image to the stream.
// Arguments:
//   image - Bitmap to save
//   output - Output stream
// Return Value:
//   true If saving was successful
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2PngHandler::Save( const ImageIO::HostBitmap& image, ICcpStream* output )
{
	if( !image.IsValid() )
	{
		CCP_LOGWARN( "Tr2PngHandler::Save input image isn't valid" );
		return false;
	}

	if( !IsSaveSupported( image ) )
	{
		CCP_LOGWARN( "Tr2PngHandler::Save does not support this image format %d", image.GetFormat() );
		return false;
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
		CCP_LOGERR( "TriPNGHandler couldn't create write structure (%S)", m_sourceName.c_str() );
		return false;
	}

	png_set_write_fn( png, output, &WriteDataThunk, &FlushData );
	png_set_error_fn( png, this, &Tr2PngHandler::UserError_thunk, &Tr2PngHandler::UserWarning_thunk );

	png_infop info = png_create_info_struct( png );
	if( !info )
	{
		CCP_LOGERR( "TriPNGHandler couldn't create info structure (%S)", m_sourceName.c_str() );
		return false;
	}

	int colorType;
	int bitDepth = 8;
	int transforms = PNG_TRANSFORM_BGR;
	switch( image.GetFormat() ) 
	{
	case PIXEL_FORMAT_R8_UNORM:
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
	if( g_isR10G10B10FormatInverted )
	{
		rSwizzle = 2;
		bSwizzle = 0;
	}

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

	png_set_rows( png, info, rows );

	bool result = true;

	if( !DoSaveImage( png, info, transforms ) )
	{
		CCP_LOGERR( "TriPNGHandler couldn't write PNG (%S)", m_sourceName.c_str() );
		result = false;
	}

	if( colorType == PNG_COLOR_TYPE_RGB )
	{
		for( unsigned i = 0; i < image.GetHeight(); ++i )
		{
			CCP_DELETE[] rows[i];
		}
	}
	CCP_DELETE[] rows;

	png_destroy_write_struct( &png, &info );
	return result;
}
