#include "StdAfx.h"
#include "Tr2ImageHandler.h"

using namespace Tr2RenderContextEnum;

#include "Tr2PngHandler.h"
#include "Tr2DdsHandler.h"
#include "Tr2JpgHandler.h"
#include "Tr2TgaHandler.h"
#include "Tr2BmpHandler.h"
#include "PsdHandler.h"

// On DX9 R10G10B10A2 has inverted R and B channels. We need to work around that
// when saving images.
bool g_isR10G10B10FormatInverted = false;
// On DX11 A8L8 format don't exist so we need to convert files with this format
// to B8G8R8A8.
bool g_convertA8L8FormatToB8G8R8A8 = false;

// factory helper to distinguish supported file formats.
Tr2ImageHandler *CreateImageHandler( const std::wstring &filePath )
{
	if( filePath.size() > 3 )
	{
		const wchar_t* ext = filePath.c_str() + filePath.size() - 3;
		if( ( ext[0] == L'p' || ext[0] == L'P' ) &&
			( ext[1] == L'n' || ext[1] == L'N' ) &&
			( ext[2] == L'g' || ext[2] == L'G' ) )
		{
			return CCP_NEW("Tr2ImageHandler/pngHandler") Tr2PngHandler( filePath.c_str() );
		}
		if( ( ext[0] == L'j' || ext[0] == L'J' ) &&
			( ext[1] == L'p' || ext[1] == L'P' ) &&
			( ext[2] == L'g' || ext[2] == L'G' ) )
		{
			return CCP_NEW("Tr2ImageHandler/jpgHandler") Tr2JpgHandler( filePath.c_str() );
		}

		if( ( ext[0] == L't' || ext[0] == L'T' ) &&
			( ext[1] == L'g' || ext[1] == L'G' ) &&
			( ext[2] == L'a' || ext[2] == L'A' ) )
		{
			return CCP_NEW("Tr2ImageHandler/tgaHandler") Tr2TgaHandler( filePath.c_str() );
		}

		if( ( ext[0] == L'b' || ext[0] == L'B' ) &&
			( ext[1] == L'm' || ext[1] == L'M' ) &&
			( ext[2] == L'p' || ext[2] == L'P' ) )
		{
			return CCP_NEW("Tr2ImageHandler/bmpHandler") Tr2BmpHandler( filePath.c_str() );
		}

		if( ( ext[0] == L'p' || ext[0] == L'P' ) &&
			( ext[1] == L's' || ext[1] == L'S' ) &&
			( ext[2] == L'd' || ext[2] == L'D' ) )
		{
			return CCP_NEW("Tr2ImageHandler/psdHandler") ImageIO::PsdHandler( filePath.c_str() );
		}

		if( filePath.size() > 4 )
		{
			ext = filePath.c_str() + filePath.size() - 4;
			if( ( ext[0] == L'j' || ext[0] == L'J' ) &&
				( ext[1] == L'p' || ext[1] == L'P' ) &&
				( ext[2] == L'e' || ext[2] == L'E' ) &&
				( ext[3] == L'g' || ext[3] == L'G' ) )
			{
				return CCP_NEW("Tr2ImageHandler/jpegHandler") Tr2JpgHandler( filePath.c_str() );
			}
		}
	}

	return CCP_NEW("Tr2ImageHandler/ddsHandler") Tr2DdsHandler( filePath.c_str() );
}


Tr2ImageHandler::Tr2ImageHandler( const wchar_t* sourceName ) :
	m_cutoutX( 0.0f ),
	m_cutoutY( 0.0f ),
	m_cutoutWidth( 1.0f ),
	m_cutoutHeight( 1.0f ),
	m_data( 0 ),
	m_dataSize( 0 ),
	m_mipLevelSkipCount( 0 ),
	m_mipLevelMaxCount( std::numeric_limits<uint32_t>::max() ),
	m_width( 0 ),
	m_height( 0 ),
	m_bitsPerPixel( 0 ),
	m_mipLevelCount( 1 ),
	m_volumeDepth( 1 ),
	m_desiredFormat( PIXEL_FORMAT_UNKNOWN )
{
	if( sourceName )
	{
		m_sourceName = sourceName;
	}
}

Tr2ImageHandler::~Tr2ImageHandler()
{
	if( m_data )
	{
		CCP_FREE( m_data );
		m_data = 0;
	}
}

unsigned int Tr2ImageHandler::GetMipLevelCount() const
{
	return m_mipLevelCount;
}

unsigned Tr2ImageHandler::GetWidth() const
{
	return m_width;
}

unsigned Tr2ImageHandler::GetHeight() const
{
	return m_height;
}

unsigned int Tr2ImageHandler::GetDepth() const
{
	return m_volumeDepth;
}

unsigned int Tr2ImageHandler::GetFaceSize() const
{
	const unsigned count = GetMipLevelCount();
	if( count == 0 )
	{
		return GetMipLevelSize( 0 );
	}

	unsigned int size = 0;
	for( unsigned int m = 0; m < count; ++m )
	{
		size += GetMipLevelSize( m );
	}	

	return size;
}

uint32_t Tr2ImageHandler::GetMipLevelSize( uint32_t mipmap ) const
{
	uint32_t w = GetWidth();
	uint32_t h = GetHeight();
	uint32_t d = GetDepth();

	for (uint32_t m = 0; m < mipmap; m++)
	{
		w = std::max(1U, w / 2);
		h = std::max(1U, h / 2);
		d = std::max(1U, d / 2);
	}

	if( IsCompressed() )
	{
		w = (w + 3) / 4;
		h = (h + 3) / 4;
		return GetBlockByteSize() * w * h * d;
	}
	else
	{
		// Align pixels to bytes.
		uint32_t byteCount = (m_bitsPerPixel + 7) / 8;

		return w * h * d * byteCount;
	}
}

uint32_t Tr2ImageHandler::GetMipLevelHeight( uint32_t mipmap ) const
{
	uint32_t h = GetHeight();
	
	for (uint32_t m = 0; m < mipmap; m++)
	{
		h = std::max(1U, h / 2);
	}

	if( IsCompressed() )
	{
		h = (h + 3) / 4;
	}
	return h;
}

uint32_t Tr2ImageHandler::GetMipLevelDepth( uint32_t mipLevel ) const
{
	uint32_t d = GetDepth();
	
	for (uint32_t m = 0; m < mipLevel; m++)
	{
		d = d / 2;
	}
	return std::max( 1U, d );
}

uint32_t Tr2ImageHandler::GetMipLevelWidth( uint32_t mipmap ) const
{
	uint32_t w = GetWidth();
	w >>= mipmap;
	w = std::max( 1U, w );

	if( IsCompressed() )
	{
		w = (w + 3) / 4;
	}
	return w;
}

unsigned Tr2ImageHandler::GetPitch( unsigned mipLevel )
{
	if( IsCompressed() )
	{
		return GetMipLevelSize( mipLevel ) / GetMipLevelHeight( mipLevel ) / GetMipLevelDepth( mipLevel );
	}
	
	return GetMipLevelWidth( mipLevel ) * m_bitsPerPixel / 8;
}

void Tr2ImageHandler::SetDesiredFormat( Tr2RenderContextEnum::PixelFormat format )
{
	m_desiredFormat = format;
}

bool Tr2ImageHandler::ConvertDesiredFormat()
{
	if( m_desiredFormat == PIXEL_FORMAT_UNKNOWN ||
		m_desiredFormat == GetFormat() )
	{
		return true;
	}

	// Support the texture atlas by converting 24 to 32 bit.
	if( m_desiredFormat == PIXEL_FORMAT_B8G8R8A8_UNORM &&
		GetFormat()     == PIXEL_FORMAT_B8G8R8X8_UNORM &&
		m_mipLevelCount == 1 )
	{
		for( unsigned j = 0; j != m_height; ++j )
		{
			unsigned char* in  = m_data  + j * m_width * 4;
			for( unsigned i = 0; i != m_width; ++i )
			{
				in[3] = 0xFF;
				in += 4;
			}
		}
		return true;
	}

	if( m_mipLevelCount >= 1 )
	{
		if( ( m_desiredFormat == PIXEL_FORMAT_B8G8R8A8_UNORM && GetFormat() == PIXEL_FORMAT_R8_UNORM   ) ||
			( m_desiredFormat == PIXEL_FORMAT_B8G8R8A8_UNORM && GetFormat() == PIXEL_FORMAT_R8G8_UNORM ) )
		{
			unsigned totalSize = 0;
			for( unsigned mip = 0; mip != m_mipLevelCount; ++mip )
			{
				totalSize += std::max( m_width >> mip, 1u ) * std::max( m_height >> mip, 1u ) * 4;
			}
		
			unsigned char* newData = (unsigned char*)CCP_MALLOC( "Tr2ImageHandler/m_data", totalSize );
			if( !newData )
			{
				CCP_LOGERR( "Tr2ImageHandler::ConvertDesiredFormat couldn't allocate %d bytes", totalSize );
				return false;
			}

			const unsigned char* src = m_data;
				  unsigned char* dst = newData;

			unsigned width = m_width;
			unsigned height = m_height;

			const bool A8L8 = GetFormat() == PIXEL_FORMAT_R8G8_UNORM;

			for( unsigned mip = 0; mip != m_mipLevelCount; ++mip )
			{
				for( unsigned j = 0; j != height; ++j )
				{
					const unsigned char* in  = src + j * width * ( A8L8 ? 2 : 1 );
						  unsigned char* out = dst + j * width * 4;
					for( unsigned i = 0; i != width; ++i )
					{
						*out++ = in[0];
						*out++ = in[0];
						*out++ = in[0];
						*out++ = A8L8 ? in[1] : 0xFF;
						in += A8L8 ? 2 : 1;
					}
				}

				src += width * height * ( A8L8 ? 2 : 1 );
				dst += width * height * 4;

				width  = std::max( width / 2, 1u );
				height = std::max( height/ 2, 1u );
			}

			CCP_FREE( m_data );
			m_data = newData;
			m_bitsPerPixel = 32;
						
			return true;
		}
	}

	return false;
}

void Tr2ImageHandler::AddMargin(	const Tr2RenderContextEnum::PixelFormat format,
									const unsigned char* source,
									const unsigned width, const unsigned height,
									const unsigned margin,
									std::vector<unsigned char> &output, 
									unsigned &outputPitch )
{
	const unsigned char* src = source;

	if( IsCompressedFormat( format ) )
	{
		const unsigned blockByteSize = Tr2RenderContextEnum::GetBlockByteSize( format );
		const unsigned blockPixelSize = 4;
		CCP_ASSERT( blockByteSize != 0 && margin % blockPixelSize == 0 );
		const unsigned blockMargin	= margin	/ blockPixelSize;
		const unsigned blocksX		= width		/ blockPixelSize;
		const unsigned blocksY		= height	/ blockPixelSize;

		//technically the block contents need to be mirrored in the margin to get nice filtering,
		// but that's somewhat fiddly.
		//textures which are intended to be used in a tiled fashion could also use the block from
		// the opposite edge.
		//maybe we need a usage hint here to inform this decision.
		
		outputPitch = ( blocksX + 2 * blockMargin ) * blockByteSize;
		output.resize( ( blockMargin + blocksY + blockMargin ) * outputPitch );
		unsigned char* dst = &output[0];

		//top margin
		for( unsigned i = 0; i < blockMargin; ++i )
		{
			memcpy( dst + blockMargin * blockByteSize, src, blocksX * blockByteSize );
			dst += outputPitch;
		}

		// Have to copy one line at a time since the target area is not linearly laid out.
		
		for( unsigned line = 0; line < height; line += blockPixelSize )
		{
			//left margin
			for( unsigned i = 0; i != blockMargin; ++i ) 
			{
				memcpy( dst + i * blockByteSize, src, blockByteSize );
			}
			
			memcpy( dst + blockMargin * blockByteSize, src, blocksX * blockByteSize );
			
			//right margin
			for( unsigned i = 0; i != blockMargin; ++i ) 
			{
				memcpy( dst + (blocksX+blockMargin+i) * blockByteSize, src + (blocksX-1) * blockByteSize, blockByteSize );
			}

			if( line < height-1 )
			{
				src += blocksX * blockByteSize;
			}
		}

		//bottom margin
		for( unsigned i = 0; i != blockMargin; ++i )
		{
			memcpy( dst + blockMargin * blockByteSize, src, blocksX * blockByteSize );
			dst += outputPitch;
		}

		CCP_ASSERT( dst == &output[0] + output.size() );
	}
	else
	{
		// Align pixels to bytes.
		const unsigned byteCount = GetBytesPerPixel( format );

		// Align srcPitch to 4 bytes.
		unsigned int srcPitch = 4 * ((width * byteCount + 3) / 4);

		outputPitch = ( width + 2 * margin ) * byteCount;

		output.resize( ( height + 2 * margin ) * outputPitch );
		unsigned char* dst = &output[0];

		//top margin
		for( unsigned i = 0; i != margin; ++i )
		{
			//note that we don't touch the corners - may want to stick debug colours in these texels,
			// at least for margins > 1. Hmm.
			memcpy( dst + margin * byteCount, src, width * byteCount );
			dst += outputPitch;
		}

		// Have to copy one line at a time since the target area is not linearly laid out.
		for( unsigned line = 0; line != height; ++line )
		{
			//left margin
			for( unsigned i = 0; i != margin; ++i ) 
			{
				memcpy( dst + i * byteCount, src, byteCount );
			}

			memcpy( dst + margin * byteCount, src, width * byteCount );
			
			//right margin
			for( unsigned i = 0; i != margin; ++i ) 
			{
				memcpy( dst + ( width + margin + i ) * byteCount, src + ( width-1 ) * byteCount, byteCount );
			}

			if( line < height-1 )
			{
				src += srcPitch;
			}
			dst += outputPitch;
		}

		//bottom margin
		for( unsigned i = 0; i != margin; ++i )
		{
			memcpy( dst + margin * byteCount, src, width * byteCount );
			dst += outputPitch;
		}

		CCP_ASSERT( dst == &output[0] + output.size() );
	}
}

unsigned int Tr2ImageHandler::GetTotalDataSize() const
{
	if( IsCubeTexture() )
	{
		return 6 * GetFaceSize();
	}
	else
	{
		unsigned int n = GetMipLevelCount();
		if( n == 0 )
		{
			n = 1;
		}

		unsigned int size = 0;
		for( unsigned int i = 0; i < n ; ++i )
		{
			size += GetMipLevelSize( i );
		}

		return size;
	}		
}

bool Tr2ImageHandler::Convert24BitTo32Bit()
{
	if( m_bitsPerPixel == 32 )
	{
		return true;
	}
	if( m_bitsPerPixel != 24 )
	{
		return false;
	}

	const unsigned oldSize = GetTotalDataSize();

	m_bitsPerPixel = 32;

	const unsigned int newSize = GetTotalDataSize();
	unsigned char* newData = (unsigned char*)CCP_MALLOC( "Tr2ImageHandler/m_data", newSize );
	if( !newData )
	{
		CCP_LOGERR( "Tr2ImageHandler::Convert24BitTo32Bit couldn't allocate %d bytes", newSize );
		return false;
	}

	unsigned char* end = m_data + oldSize;
	unsigned char* src = m_data;
	unsigned char* dst = newData;
	unsigned char* dstEnd = newData + newSize;
	while( src < end )
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = 0;

		if( (dst >= dstEnd) && (src < end) )
		{
			break;
		}
	}

	CCP_FREE( m_data );
	m_data = newData;

	return true;
}

void Tr2ImageHandler::SetMipLevelSkipCount( unsigned int skip )
{
	m_mipLevelSkipCount = skip;
}

void Tr2ImageHandler::SetMipLevelMaxCount( unsigned int max )
{
	m_mipLevelMaxCount = max;
}

void Tr2ImageHandler::GetMipLevelRange( unsigned & skipCount, unsigned & mipCount )
{
	skipCount = 0;
	
	if( m_width > 8 && m_height > 8 )
	{
		if( mipCount > m_mipLevelSkipCount )
		{
			skipCount = m_mipLevelSkipCount;
		}
		else if( mipCount )
		{
			// limit total mipmap count to three!
			skipCount = mipCount - 3;
		}
		else
		{
			// mipCount == 0 is a special case where the driver is asked to autogenerate the mip maps.
			// We cannot skip the first n in the chain because we don't have a chain to load!
		}
	}

	mipCount -= skipCount;
	if( mipCount > m_mipLevelMaxCount )
	{
		skipCount += (mipCount - m_mipLevelMaxCount);
		mipCount = m_mipLevelMaxCount;
	}
}

unsigned char* Tr2ImageHandler::GetMipBytes( unsigned mipLevel, unsigned face )
{
	return m_data ? m_data + GetOffset( mipLevel, face ) : NULL;
}

const wchar_t* Tr2ImageHandler::GetSourceName() const
{
	return m_sourceName.c_str();
}