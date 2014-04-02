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

extern bool g_isR10G10B10FormatInverted;

struct Tr2JpgHandler::Impl
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

using namespace Tr2RenderContextEnum;

//callbacks for libjpeg source and error handlers
namespace 
{
	//source manager functions
	void init_source(j_decompress_ptr cinfo)
	{
		Tr2JpgHandler::InputData *client = (Tr2JpgHandler::InputData *)cinfo->client_data;
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

		Tr2JpgHandler::InputData *client = (Tr2JpgHandler::InputData *)cinfo->client_data;
		longjmp( client->m_jmpBuf, 1 );
	}

	void emit_message( j_common_ptr, int )
	{
		//char buffer[JMSG_LENGTH_MAX];
		//cinfo->err->format_message( cinfo, buffer );
		//CCP_LOG( "libjpeg[%d]: %08xd : %s", msg_level, cinfo->err->msg_code, buffer );
	}

	void output_message( j_common_ptr )
	{
		//char buffer[JMSG_LENGTH_MAX];
		//cinfo->err->format_message( cinfo, buffer );
		//CCP_LOG( "libjpeg[-]:  %08xd : %s", cinfo->err->msg_code, buffer );
	}
	void reset_error_mgr( j_common_ptr )
	{
		// CCP_LOG( "libjpeg called reset_error_mgr" );
	}
}

Tr2JpgHandler::Tr2JpgHandler( const wchar_t* sourceName )
:	Tr2ImageHandler( sourceName ),
	m_impl( new Impl ),
	m_rgbData( "Tr2JpgHandler/m_rgbData" ),
	m_compressedData( "Tr2JpgHandler/m_compressedData" ),
	m_channels( 0 )
{
	if( setjmp( m_clientData.m_jmpBuf ) )
	{
		return;
	}

	jpeg_std_error( &m_impl->m_jpegError );
	m_impl->m_jpegError.reset_error_mgr = reset_error_mgr;
	m_impl->m_jpegError.output_message  = output_message;
	m_impl->m_jpegError.error_exit      = error_exit;
	m_impl->m_jpegError.emit_message    = emit_message;
	m_impl->m_decode.err = &m_impl->m_jpegError;
	jpeg_create_decompress( &m_impl->m_decode );

	m_data = NULL;

	//we always expand rgb and greyscale to argb8
	m_bitsPerPixel = 32;
}

Tr2JpgHandler::~Tr2JpgHandler()
{
	CCP_DELETE[] m_data;
	m_data = NULL;

	if( setjmp( m_clientData.m_jmpBuf ) )
	{
		return;
	}

	jpeg_destroy_decompress( &m_impl->m_decode );
}

bool Tr2JpgHandler::ReadHeader( ICcpStream* stream )
{
	if( stream->GetSize() >= 0 )
	{
		m_compressedData.resize( stream->GetSize() );
	}
	if( m_compressedData.empty() )
	{
		return false;
	}

	stream->Read( &m_compressedData[0], m_compressedData.size() ); 

	m_clientData.start = &m_compressedData[0];
	m_clientData.dataSize = (uint32_t)m_compressedData.size();
	m_impl->m_decode.client_data = (void*)&m_clientData;

	m_impl->m_sourceManager.init_source       = init_source;
	m_impl->m_sourceManager.fill_input_buffer = fill_input_buffer;
	m_impl->m_sourceManager.resync_to_restart = NULL;
	m_impl->m_sourceManager.skip_input_data   = skip_input_data;
	m_impl->m_sourceManager.term_source       = term_source;
	m_impl->m_sourceManager.next_input_byte   = &m_compressedData[0];
	m_impl->m_sourceManager.bytes_in_buffer   = m_compressedData.size();

	m_impl->m_decode.src = &m_impl->m_sourceManager;

	if( setjmp( m_clientData.m_jmpBuf ) )
	{
		return false;
	}

	// jpeg files can contain multiple Start of Image tags, usually because the first is a thumbnail
	// not sure of the best way to handle this yet.
	int result = jpeg_read_header( &m_impl->m_decode, false );
	if( result == JPEG_HEADER_OK ) 
	{
		if( jpeg_has_multiple_scans( &m_impl->m_decode ) )
		{
			CCP_LOG( "jpeg has multiple scans" );
		}
		m_height	= m_impl->m_decode.image_height;
		m_width		= m_impl->m_decode.image_width;
		m_channels	= m_impl->m_decode.num_components;
		//CCP_LOG( "JPEG header read: %d x %d x %d", m_width, m_height, m_channels );
	} 
	else 
	{
		CCP_LOGERR( "Failed to read JPEG header from %S", m_sourceName.c_str() );
		return false;
	}
	
	return true;
}

Tr2RenderContextEnum::PixelFormat Tr2JpgHandler::GetFormat() const 
{
	return Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM;
}

bool Tr2JpgHandler::ReadImage( ICcpStream* stream )
{
	if( m_height > 4096 || m_width > 4096 ) 
	{
		CCP_LOGWARN( "Very large jpeg image being loaded: %d x %d", m_width, m_height );
	}

	//initialise storage for decompressed data
	m_rgbData.resize( m_height * m_width * m_channels );
	if( m_rgbData.empty() )
	{
		return false;
	}

	memset( &m_rgbData[0], 0xff, m_height * m_width * m_channels );
	
	m_data = CCP_NEW("Tr2JpgHandler/m_data") unsigned char[ m_height * m_width * 4 ];
	if( !m_data )
	{
		m_rgbData.clear();
		return false;
	}

	if( setjmp( m_clientData.m_jmpBuf ) )
	{
		CCP_DELETE [] m_data;
		m_data = nullptr;
		return false;
	}

	memset( m_data, 0x00, m_height * m_width * 4 );

	//decompression, one scanline at a time
	jpeg_calc_output_dimensions( &m_impl->m_decode );
	if( !jpeg_start_decompress( &m_impl->m_decode ) )
	{
		CCP_LOGERR( "jpeg_start_decompress failed!" );
		return false;
	}

	for( unsigned y = 0; y < m_height; ++y )
	{
		unsigned char *line = &m_rgbData[ y * m_width * m_channels ];
		int lines = jpeg_read_scanlines( &m_impl->m_decode, &line, 1 );
		if( lines != 1 )
		{
			CCP_LOGERR( "jpeg_read_scanlines failed (%d : %d)", y, lines );
			return false;
		}
	}
	jpeg_input_complete( &m_impl->m_decode );

	if( !jpeg_finish_decompress( &m_impl->m_decode ) )
	{
		CCP_LOGERR( "jpeg_finish_decompress failed!" );
		return false;
	}


	//expand RGB8 or L8 to ARGB8
	for( unsigned y=0; y<m_height; ++y )
	{
		unsigned char *src = &m_rgbData[ y * m_width * m_channels ];
		unsigned char *dst = m_data + (y * m_width * 4);
		for( unsigned x=0; x<m_width; ++x )
		{
			switch( m_channels )
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

	return true;
}

unsigned Tr2JpgHandler::GetBlockByteSize() const
{
	//not compressed
	return 0;
}

unsigned Tr2JpgHandler::GetOffset( unsigned, unsigned ) const
{
	return 0;
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

bool Tr2JpgHandler::IsSaveSupported( const Tr2BitmapDimensions& bd )
{
	if( bd.GetType() != TEX_TYPE_2D )
	{
		CCP_LOGWARN( "Tr2JpgHandler::Save does not support this texture tyep: %d", bd.GetType() );
		return false;
	}

	if( bd.GetFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM		&&
		bd.GetFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM		&& 
		bd.GetFormat() != PIXEL_FORMAT_R8_UNORM				&&
		bd.GetFormat() != PIXEL_FORMAT_R10G10B10A2_UNORM	&&
		bd.GetFormat() != PIXEL_FORMAT_R10G10B10A2_TYPELESS )
	{
		CCP_LOGWARN( "Tr2JpgHandler::Save does not support this image format: %d", bd.GetFormat() );
		return false;
	}

	return true;
}

bool Tr2JpgHandler::Save( const ImageIO::HostBitmap& image, ICcpStream* output )
{	
	CCP_ASSERT( output );
	CCP_ASSERT( image.IsValid() );

	using namespace Tr2RenderContextEnum;

	if( !IsSaveSupported( image ) )
	{		
		return false;
	}

	TSaveData saveData;
	saveData.output = output;
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
		if( g_isR10G10B10FormatInverted )
		{
			rSwizzle = 2;
			bSwizzle = 0;
		}

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

	return true;
}
