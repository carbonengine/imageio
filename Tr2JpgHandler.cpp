// Copyright © 2014 CCP ehf.

#include "StdAfx.h"

#include "Tr2JpgHandler.h"
#include "HostBitmap.h"

//do the macro dance! (jpegcfg.h defines HAVE_STDDEF_H, as does something pythonish)
//although this didn't _seem_ to break anything, the warning is ugly.
#ifdef  HAVE_STDDEF_H
#define HAD_STDDEF_H_OVER_HERE 1
#undef  HAVE_STDDEF_H
#endif //HAVE_STDDEF_H
#ifdef  HAVE_PROTOTYPES
#define HAD_PROTOTYPES_OVER_HERE 1
#undef  HAVE_PROTOTYPES
#endif
#ifdef  HAVE_STDLIB_H
#define HAD_STDLIB_H_OVER_HERE 1
#undef  HAVE_STDLIB_H
#endif

#include <jpeglib.h>

//back to the status quo we go.
#ifdef  HAD_STDDEF_H_OVER_HERE
#undef  HAVE_STDDEF_H
#define HAVE_STDDEF_H HAD_STDDEF_H_OVER_HERE
#endif //HAD_STDDEF_H_OVER_HERE
#ifdef  HAD_PROTOTYPES_OVER_HERE
#undef  HAVE_PROTOTYPES
#define HAVE_PROTOTYPES HAD_PROTOTYPES_OVER_HERE
#endif
#ifdef  HAD_STDLIB_H_OVER_HERE
#undef  HAVE_STDLIB_H
#define HAVE_STDLIB_H HAD_STDLIB_H_OVER_HERE
#endif

using namespace ImageIO;

//callbacks for libjpeg source and error handlers
namespace 
{

struct Impl
{
	Impl()
	{
		memset( &m_decode,			0, sizeof( m_decode ) );
		memset( &m_jpegError,		0, sizeof( m_jpegError ) );
		memset( &m_sourceManager,	0, sizeof( m_sourceManager ) );
	}

	jpeg_decompress_struct m_decode;
	jpeg_source_mgr m_sourceManager;
	jpeg_error_mgr m_jpegError;
};
	
struct InputData
{
	uint8_t* start;
	unsigned dataSize;
	jmp_buf m_jmpBuf;
};

//source manager functions
void init_source(j_decompress_ptr cinfo)
{
	InputData *client = (InputData *)cinfo->client_data;
	if( !client )
	{
		CCP_LOGERR( "libjpeg - init_source: No input data");
		return;
	}
	cinfo->src->next_input_byte = client->start;
	cinfo->src->bytes_in_buffer = client->dataSize;
}

boolean fill_input_buffer(j_decompress_ptr)
{
	//I guess this is irrelevant, as we have no additional data once our buffer is empty.
	return 0;
}

void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	if( num_bytes > (long)cinfo->src->bytes_in_buffer )
	{
		num_bytes = (long)cinfo->src->bytes_in_buffer;
	}
	cinfo->src->next_input_byte += num_bytes;
	cinfo->src->bytes_in_buffer -= num_bytes;
}

void term_source(j_decompress_ptr)
{
	//I guess this is used to clean up anything after reading, but we handle that externally.
}


//error manager functions
//a lot of these seem extremely spammy, but if they're not provided
//libjpeg will tend to kill the process.
void error_exit( j_common_ptr cinfo )
{
	char buffer[JMSG_LENGTH_MAX];
	cinfo->err->format_message( cinfo, buffer );
	CCP_LOGERR( "libjpeg[x]: %08xd : %s", cinfo->err->msg_code, buffer );

	InputData *client = (InputData *)cinfo->client_data;
	longjmp( client->m_jmpBuf, 1 );
}

void emit_message( j_common_ptr, int )
{
}

void output_message( j_common_ptr )
{
}

void reset_error_mgr( j_common_ptr )
{
}

ImageIO::Result ReadImage( ImageIO::HostBitmap& bitmap, unsigned channels, InputData& clientData, Impl& impl )
{
	//initialise storage for decompressed data
	CcpMallocBuffer rgbData( "Tr2JpgHandler::rgbData", bitmap.GetWidth() * bitmap.GetHeight() * channels );
	if( rgbData.empty() )
	{
		return ImageIO::Result::OUT_OF_MEMORY;
	}

	memset( rgbData.get(), 0xff, bitmap.GetWidth() * bitmap.GetHeight() * channels );

	if( setjmp( clientData.m_jmpBuf ) )
	{
		return ImageIO::Result::INVALID_DATA;
	}

	memset( bitmap.GetRawData(), 0x00, bitmap.GetWidth() * bitmap.GetHeight() * 4 );

	//decompression, one scanline at a time
	jpeg_calc_output_dimensions( &impl.m_decode );
	if( !jpeg_start_decompress( &impl.m_decode ) )
	{
		CCP_LOGERR( "jpeg_start_decompress failed!" );
		return ImageIO::Result::INVALID_DATA;
	}

	for( unsigned y = 0; y < bitmap.GetHeight(); ++y )
	{
		unsigned char *line = reinterpret_cast<unsigned char*>( rgbData.get() + y * bitmap.GetWidth() * channels );
		int lines = jpeg_read_scanlines( &impl.m_decode, &line, 1 );
		if( lines != 1 )
		{
			CCP_LOGERR( "jpeg_read_scanlines failed (%d : %d)", y, lines );
			return ImageIO::Result::INVALID_DATA;
		}
	}
	jpeg_input_complete( &impl.m_decode );

	if( !jpeg_finish_decompress( &impl.m_decode ) )
	{
		CCP_LOGERR( "jpeg_finish_decompress failed!" );
		return ImageIO::Result::INVALID_DATA;
	}

	//expand RGB8 or L8 to ARGB8
	for( unsigned y = 0; y < bitmap.GetHeight(); ++y )
	{
		unsigned char *src = reinterpret_cast<unsigned char*>( rgbData.get() + y * bitmap.GetWidth() * channels );
		unsigned char *dst = reinterpret_cast<unsigned char*>( bitmap.GetRawData() ) + (y * bitmap.GetWidth() * 4);
		for( unsigned x = 0; x < bitmap.GetWidth(); ++x )
		{
			switch( channels )
			{
			case 1:
				dst[ x * 4 + 2 ] = src[ x ];
				dst[ x * 4 + 1 ] = src[ x ];
				dst[ x * 4 + 0 ] = src[ x ];
				dst[ x * 4 + 3 ] = 0xff;
				break;
			case 3:
				dst[ x * 4 + 2 ] = src[ x * 3 + 0 ];
				dst[ x * 4 + 1 ] = src[ x * 3 + 1 ];
				dst[ x * 4 + 0 ] = src[ x * 3 + 2 ];
				dst[ x * 4 + 3 ] = 0xff;
				break;
			default:
				break;
			}
		}
	}

	return ImageIO::Result::OK;
}
}


namespace {

	struct TSaveData
	{
		std::vector<unsigned char> buffer;
		ICcpStream*	output;
	};
	
	void blue_stream_init_destination( j_compress_ptr cinfo )
	{
		TSaveData& sd = *static_cast<TSaveData*>( cinfo->client_data );
		cinfo->dest->next_output_byte = &sd.buffer[0];
		cinfo->dest->free_in_buffer   = sd.buffer.size();
	}

	boolean blue_stream_empty_output_buffer( j_compress_ptr cinfo )
	{ 
		TSaveData& sd = *static_cast<TSaveData*>( cinfo->client_data );
		sd.output->Write( &sd.buffer[0], sd.buffer.size() );
		cinfo->dest->next_output_byte = &sd.buffer[0];
		cinfo->dest->free_in_buffer   = sd.buffer.size();
		return true;
	}

	void blue_stream_term_destination( j_compress_ptr cinfo )
	{ 
		TSaveData& sd = *static_cast<TSaveData*>( cinfo->client_data );
		sd.output->Write( &sd.buffer[0], sd.buffer.size() - cinfo->dest->free_in_buffer );
		cinfo->dest->free_in_buffer = 0;
	}	
}


namespace ImageIO
{
namespace Jpeg
{

// --------------------------------------------------------------------------------------
// Description:
//   Registers JPEG handler with ImageIO.
// --------------------------------------------------------------------------------------
void RegisterHandler()
{
	static bool s_registered = false;
	if( !s_registered )
	{
		ImageFormatFunctions funcs = { &IsJpegExtension, &ReadImage, &IsSaveSupported, &Save };
		RegisterImageHandler( funcs );
		s_registered = true;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if provided extension (without leading dot) is JPEG extension.
// Arguments:
//   ext - File extension
// Return Value:
//   true If provided extension is JPEG extension
// --------------------------------------------------------------------------------------
bool IsJpegExtension( const wchar_t* ext )
{
	return ( ( ext[0] == L'j' || ext[0] == L'J' ) &&
		( ext[1] == L'p' || ext[1] == L'P' ) &&
		( ext[2] == L'g' || ext[2] == L'G' ) &&
		ext[3] == 0 ) ||
		( ( ext[0] == L'j' || ext[0] == L'J' ) &&
		( ext[1] == L'p' || ext[1] == L'P' ) &&
		( ext[2] == L'e' || ext[2] == L'E' ) &&
		( ext[3] == L'g' || ext[3] == L'G' ) &&
		ext[4] == 0 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads JPEG image from the stream.
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
	if( stream.GetSize() == 0 )
	{
		return Result::READ_FAILURE;
	}
	CcpMallocBuffer compressedData( "Tr2JpgHandler::compressedData", stream.GetSize() );
	if( compressedData.empty() )
	{
		return Result::OUT_OF_MEMORY;
	}

	stream.Read( compressedData.get(), compressedData.size() ); 

	InputData clientData;
	clientData.start = reinterpret_cast<uint8_t*>( compressedData.get() );
	clientData.dataSize = (uint32_t)compressedData.size();

	if( setjmp( clientData.m_jmpBuf ) )
	{
		return Result::ERROR_INITIALIZING_EXTERNAL_LIBRARY;
	}

	Impl impl;
	jpeg_std_error( &impl.m_jpegError );
	impl.m_jpegError.reset_error_mgr = reset_error_mgr;
	impl.m_jpegError.output_message  = output_message;
	impl.m_jpegError.error_exit      = error_exit;
	impl.m_jpegError.emit_message    = emit_message;
	impl.m_decode.err = &impl.m_jpegError;
	impl.m_decode.client_data = &clientData;
	jpeg_create_decompress( &impl.m_decode );

	ON_BLOCK_EXIT( [&] {
		if( setjmp( clientData.m_jmpBuf ) )
		{
			return;
		}
		jpeg_destroy_decompress( &impl.m_decode );
	} );

	impl.m_decode.client_data = (void*)&clientData;

	impl.m_sourceManager.init_source       = init_source;
	impl.m_sourceManager.fill_input_buffer = fill_input_buffer;
	impl.m_sourceManager.resync_to_restart = NULL;
	impl.m_sourceManager.skip_input_data   = skip_input_data;
	impl.m_sourceManager.term_source       = term_source;
	impl.m_sourceManager.next_input_byte   = reinterpret_cast<uint8_t*>( compressedData.get() );
	impl.m_sourceManager.bytes_in_buffer   = compressedData.size();

	impl.m_decode.src = &impl.m_sourceManager;

	if( setjmp( clientData.m_jmpBuf ) )
	{
		return Result::INVALID_HEADER;
	}

	unsigned channels = 0;
	// jpeg files can contain multiple Start of Image tags, usually because the first is a thumbnail
	// not sure of the best way to handle this yet.
	int result = jpeg_read_header( &impl.m_decode, false );
	if( result == JPEG_HEADER_OK ) 
	{
		uint32_t height = impl.m_decode.image_height;
		uint32_t width = impl.m_decode.image_width;
		channels = impl.m_decode.num_components;
		bitmap.Create( width, height, 1, PIXEL_FORMAT_B8G8R8A8_UNORM );
	} 
	else 
	{
		return Result::INVALID_HEADER;
	}

	if( metadata )
	{
		metadata->cutout = Cutout();
	}
	
	auto r = ::ReadImage( bitmap, channels, clientData, impl );
	if( !r )
	{
		bitmap.Destroy();
		return r;
	}
	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if saving an image into JPEG format is supported.
// Arguments:
//   dimensions - Image dimensions/type/format
// Return Value:
//   Result of the operation (OK if image saving is supported)
// --------------------------------------------------------------------------------------
Result IsSaveSupported( const BitmapDimensions& bd )
{
	if( bd.GetType() != TEX_TYPE_2D || bd.GetArraySize() != 1 )
	{
		return Result::SAVE_NOT_SUPPORTED;
	}

	if( bd.GetFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM		&&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM		&& 
		bd.GetFormat() != PIXEL_FORMAT_R8_UNORM				&&
		bd.GetFormat() != PIXEL_FORMAT_R10G10B10A2_UNORM	&&
		bd.GetFormat() != PIXEL_FORMAT_R10G10B10A2_TYPELESS )
	{
		return Result::SAVE_NOT_SUPPORTED;
	}

	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves a bitmap to JPEG file.
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

	IMAGE_IO_CR_RETURN_RESULT( IsSaveSupported( image ) );

	TSaveData saveData;
	saveData.output = &output;
	saveData.buffer.resize( 65536 );



	jpeg_compress_struct	cinfo = { 0 };
	jpeg_error_mgr			jerr  = { 0 };
	jpeg_destination_mgr	mgr   = { 0 };

	cinfo.err = jpeg_std_error( &jerr );	
	
	mgr.init_destination	= blue_stream_init_destination;
	mgr.empty_output_buffer	= blue_stream_empty_output_buffer;
	mgr.term_destination	= blue_stream_term_destination;

	jpeg_create_compress( &cinfo );
	

	cinfo.dest = &mgr;

	cinfo.client_data = &saveData;

	cinfo.image_width		= image.GetWidth();
	cinfo.image_height		= image.GetHeight();
	cinfo.input_components	= image.GetFormat() == PIXEL_FORMAT_R8_UNORM ? 1 : 3;
	cinfo.in_color_space	= image.GetFormat() == PIXEL_FORMAT_R8_UNORM ? JCS_GRAYSCALE : JCS_RGB;
	jpeg_set_defaults( &cinfo );
	cinfo.dct_method		= JDCT_FLOAT;
	jpeg_set_quality( &cinfo, 90, TRUE );

	jpeg_start_compress( &cinfo, FALSE );
	

	JSAMPROW row_pointer[1];

	if( image.GetFormat() == PIXEL_FORMAT_R8_UNORM )
	{
		for( unsigned j = 0; j != image.GetHeight(); ++j )
		{
			row_pointer[0] = (unsigned char*)image.GetRawData( 0, j );
			jpeg_write_scanlines( &cinfo, row_pointer, 1 );
		}
	}
	else
	{
		std::vector<unsigned char> rgb( image.GetWidth() * 3 );
		row_pointer[0] = &rgb[0];

		int rSwizzle = 0;
		int bSwizzle = 2;

		const bool is1010102 = 
			image.GetFormat() == PIXEL_FORMAT_R10G10B10A2_TYPELESS ||
			image.GetFormat() == PIXEL_FORMAT_R10G10B10A2_UNORM;

		for( unsigned j = 0; j != image.GetHeight(); ++j )
		{
			const uint8_t* src = (const uint8_t*)image.GetRawData( 0, j );
			uint8_t* dst = &rgb[0];

			if( is1010102 )
			{
				for( unsigned i = 0; i != image.GetWidth(); ++i, dst += 3, src += 4 )
				{
					const uint32_t rgb10 = *(const uint32_t*)src;
					dst[bSwizzle] = uint8_t( ( ( rgb10 >> 20 ) & 1023 ) >> 2 );
					dst[1] = uint8_t( ( ( rgb10 >> 10 ) & 1023 ) >> 2 );
					dst[rSwizzle] = uint8_t( ( ( rgb10 >>  0 ) & 1023 ) >> 2 );
				}
			}
			else
			{
				for( unsigned i = 0; i != image.GetWidth(); ++i, dst += 3, src += 4 )
				{
					dst[0] = src[2];
					dst[1] = src[1];
					dst[2] = src[0];
				}
			}

			jpeg_write_scanlines( &cinfo, row_pointer, 1 );
		}
	}

	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );

	return Result::OK;
}

}
}