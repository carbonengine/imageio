// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "HostBitmap.h"
#include "Tr2ImageHandler.h"
#include "ImageUtility.h"

namespace
{

size_t GetDataSize( const ImageIO::BitmapDimensions& dim )
{
	uint32_t mipCount = dim.GetTrueMipCount();
	uint32_t width = dim.GetWidth();
	uint32_t height = dim.GetHeight();
	uint32_t depth = dim.GetType() == ImageIO::TEX_TYPE_3D ? dim.GetDepth() : 1;
	size_t size = 0;
	if( IsCompressedFormat( dim.GetFormat() ) )
	{
		while( mipCount-- )
		{
			size += std::max( ( width + 3u ) & ~3u, 4u ) * std::max( ( height + 3u ) & ~3u, 4u ) * depth * GetBlockByteSize( dim.GetFormat() ) / 16;
			width  = std::max( width  / 2u, 1u );
			height = std::max( height / 2u, 1u );
			depth = std::max( depth / 2u, 1u );			
		}
	}
	else
	{
		while( mipCount-- )
		{
			size += width * height * depth * GetBytesPerPixel( dim.GetFormat() );
			width  = std::max( width  / 2u, 1u );
			height = std::max( height / 2u, 1u );			
			depth = std::max( depth / 2u, 1u );			
		}		
	}
	size *= dim.GetArraySize();
	return size;
}

}

namespace ImageIO
{

HostBitmap::HostBitmap()
{
}

HostBitmap::HostBitmap( HostBitmap&& other )
	:BitmapDimensions( other )
{
	std::swap( m_name, other.m_name );
	std::swap( m_data, other.m_data );
}

HostBitmap::~HostBitmap()
{
	Destroy();
}

HostBitmap& HostBitmap::operator=( HostBitmap&& other )
{
	std::swap( static_cast<BitmapDimensions&>( *this ), static_cast<BitmapDimensions&>( other ) );
	std::swap( m_name, other.m_name );
	std::swap( m_data, other.m_data );
	return *this;
}

bool HostBitmap::Create( unsigned width, unsigned height, unsigned mipCount, PixelFormat format )
{
	Destroy();

	if( !width || !height || format >= PIXEL_FORMAT_SENTINEL )
	{
		CCP_LOGWARN( "HostBitmap::Create invalid parameters: %d x %d, %d mips, format %d", width, height, mipCount, format );
		return false;
	}

	if( IsCompressedFormat( format ) )
	{
		if( ( width % 4 ) != 0 || ( height % 4 ) != 0 )
		{
			CCP_LOGWARN( "HostBitmap::Create invalid compressed size: %d x %d", width, height );
			return false;
		}
	}

	m_width  = width;
	m_height = height;
	m_volumeDepth = 1;
	m_format = format;
	m_mipCount = mipCount;
	m_arraySize = 1;
	m_type = TEX_TYPE_2D;

	size_t size = GetDataSize( *this );

	m_data.resize( "HostBitmap::m_data", size );
	
	if( !m_data )
	{
		CCP_LOGWARN( "HostBitmap::Create failed to allocated %d bytes", size );
		Destroy();
		return false;
	}

	return true;
}

bool HostBitmap::Create2DArray( unsigned width, unsigned height, unsigned mipCount, unsigned arraySize, PixelFormat format )
{
	Destroy();

	if( !width || !height || !arraySize || format >= PIXEL_FORMAT_SENTINEL )
	{
		CCP_LOGWARN( "HostBitmap::Create2DArray invalid parameters: %d x %d, %d mips, %d array elements, format %d", width, height, mipCount, arraySize, format );
		return false;
	}

	if( IsCompressedFormat( format ) )
	{
		if( ( width % 4 ) != 0 || ( height % 4 ) != 0 )
		{
			CCP_LOGWARN( "HostBitmap::Create invalid compressed size: %d x %d", width, height );
			return false;
		}
	}

	m_width  = width;
	m_height = height;
	m_volumeDepth = 1;
	m_format = format;
	m_mipCount = mipCount;
	m_arraySize = arraySize;
	m_type = TEX_TYPE_2D;

	size_t size = GetDataSize( *this );

	m_data.resize( "HostBitmap::m_data", size );
	
	if( !m_data )
	{
		CCP_LOGWARN( "HostBitmap::Create failed to allocated %d bytes", size );
		Destroy();
		return false;
	}

	return true;
}

bool HostBitmap::CreateCube( unsigned width, unsigned mipCount, PixelFormat format )
{
	Destroy();

	if( !width || format >= PIXEL_FORMAT_SENTINEL )
	{
		return false;
	}

	if( IsCompressedFormat( format ) )
	{
		if( ( width % 4 ) != 0 )
		{
			return false;
		}
	}

	m_width  = width;
	m_height = width;
	m_volumeDepth = 1;
	m_format = format;
	m_mipCount = mipCount;
	m_type = TEX_TYPE_CUBE;
	m_arraySize = 6;

	size_t size = GetDataSize( *this );
	m_data.resize( "HostBitmap::m_data", size );
	
	return true;
}

bool HostBitmap::CreateVolume( unsigned width, unsigned height, unsigned depth, unsigned mipCount, PixelFormat format )
{
	Destroy();

	if( !width || format >= PIXEL_FORMAT_SENTINEL )
	{
		return false;
	}

	if( IsCompressedFormat( format ) )
	{
		if( ( width % 4 ) != 0 || ( height % 4 ) != 0 )
		{
			return false;
		}
	}

	m_width  = width;
	m_height = height;
	m_volumeDepth = depth;
	m_format = format;
	m_mipCount = mipCount;
	m_type = TEX_TYPE_3D;
	m_arraySize = 1;

	size_t size = GetDataSize( *this );
	m_data.resize( "HostBitmap::m_data", size );

	return true;
}

bool HostBitmap::CreateFromBitmapDimensions( const BitmapDimensions& dimensions )
{
	if( dimensions.GetWidth() == 0 || dimensions.GetFormat() == PIXEL_FORMAT_UNKNOWN )
	{
		return false;
	}
	static_cast<BitmapDimensions&>( *this ) = dimensions;
	m_data.resize( "HostBitmap::m_data", GetDataSize( *this ) );
	if( !m_data.get() )
	{
		return false;
	}
	return true;
}

bool HostBitmap::IsValid() const
{
	return !m_data.empty();
}

void HostBitmap::Destroy()
{
	BitmapDimensions::Destroy();
	
	m_data.clear();	
}

bool HostBitmap::ChangeFormat( PixelFormat format )
{
	if( !IsValid() )
	{
		return false;
	}
	if( IsCompressedFormat( format ) || IsCompressedFormat( m_format ) ||
		GetBytesPerPixel( format ) != GetBytesPerPixel( m_format ) )
	{
		return false;
	}
	m_format = format;
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Tries to convert image format. Only some transitions are supported: 
//     B8G8R8X8_UNORM <-> B8G8R8A8_UNORM;
//     R8_UNORM <-> B8G8R8A8_UNORM;
//     B8G8R8X8_UNORM -> R8_UNORM;
//     R8G8_UNORM -> B8G8R8A8_UNORM.
// Arguments:
//   format - New image format
// Return Value:
//   true When conversion was successful
//   false Otherwise
// --------------------------------------------------------------------------------------
bool HostBitmap::ConvertFormat( PixelFormat format )
{
	if( !IsValid() )
	{
		return false;
	}
	if( format == GetFormat() )
	{
		return true;
	}

	if( format == PIXEL_FORMAT_B8G8R8X8_UNORM && GetFormat() == PIXEL_FORMAT_B8G8R8A8_UNORM )
	{
		m_format = format;
		return true;
	}

	if( format == PIXEL_FORMAT_B8G8R8A8_UNORM && GetFormat() == PIXEL_FORMAT_B8G8R8X8_UNORM )
	{
		for( unsigned i = 0; i != GetRawDataSize(); i += 4 )
		{
			uint8_t* in  = reinterpret_cast<uint8_t*>( GetRawData() )  + i;
			in[3] = 0xFF;
		}
		m_format = PIXEL_FORMAT_B8G8R8A8_UNORM;
		return true;
	}

	if( format == PIXEL_FORMAT_R8_UNORM && ( GetFormat() == PIXEL_FORMAT_B8G8R8A8_UNORM || GetFormat() == PIXEL_FORMAT_B8G8R8X8_UNORM ) )
	{
		size_t pixelCount = GetRawDataSize() / GetBytesPerPixel( GetFormat() );
		uint8_t* oldData = (unsigned char*)CCP_MALLOC( "HostBitmap::ConvertFormat", GetRawDataSize() );
		if( !oldData )
		{
			return false;
		}
		ON_BLOCK_EXIT( [&] { CCP_FREE( oldData ); } );

		memcpy( oldData, m_data.get(), GetRawDataSize() );

		size_t newSize = pixelCount;
		m_data.resize( "HostBitmap::m_data", newSize );
		if( m_data.empty() )
		{
			Destroy();
			return false;
		}

		const uint8_t* src = reinterpret_cast<uint8_t*>( oldData );
		uint8_t* dst = reinterpret_cast<uint8_t*>( m_data.get() );

		for( size_t i = 0; i < pixelCount; ++i )
		{
			*dst++ = *src;
			src += 4;
		}

		m_format = format;

		return true;
	}

	if( ( format == PIXEL_FORMAT_R8G8B8A8_UNORM && ( GetFormat() == PIXEL_FORMAT_B8G8R8A8_UNORM || GetFormat() == PIXEL_FORMAT_B8G8R8X8_UNORM ) ) || 
		( GetFormat() == PIXEL_FORMAT_R8G8B8A8_UNORM && ( format == PIXEL_FORMAT_B8G8R8A8_UNORM || format == PIXEL_FORMAT_B8G8R8X8_UNORM ) ) )
	{
		char *data = m_data.get();
		size_t size = GetRawDataSize();
		unsigned bpp = GetBytesPerPixel( GetFormat() );

		for( unsigned i = 0; i < size-2; i += bpp )
		{
			char tmp = data[i];
			data[i] = data[i+2];
			data[i+2] = tmp;
		}

		m_format = format;

		return true;
	}

	if( ( format == PIXEL_FORMAT_B8G8R8A8_UNORM && ( GetFormat() == PIXEL_FORMAT_R8_UNORM || GetFormat() == PIXEL_FORMAT_R8G8_UNORM ) ) ||
		( format == PIXEL_FORMAT_B8G8R8X8_UNORM && GetFormat() == PIXEL_FORMAT_R8_UNORM ) )
	{
		size_t pixelCount = GetRawDataSize() / GetBytesPerPixel( GetFormat() );
		uint8_t* oldData = (unsigned char*)CCP_MALLOC( "HostBitmap::ConvertFormat", GetRawDataSize() );
		if( !oldData )
		{
			return false;
		}
		ON_BLOCK_EXIT( [&] { CCP_FREE( oldData ); } );

		memcpy( oldData, m_data.get(), GetRawDataSize() );

		size_t newSize = pixelCount * 4;
		m_data.resize( "HostBitmap::m_data", newSize );
		if( m_data.empty() )
		{
			Destroy();
			return false;
		}

		const uint8_t* src = reinterpret_cast<uint8_t*>( oldData );
		uint8_t* dst = reinterpret_cast<uint8_t*>( m_data.get() );

		if( GetFormat() == PIXEL_FORMAT_R8_UNORM )
		{
			for( size_t i = 0; i < pixelCount; ++i )
			{
				*dst++ = *src;
				*dst++ = *src;
				*dst++ = *src;
				++src;
				*dst++ = 0xff;
			}
		}
		else
		{
			for( size_t i = 0; i < pixelCount; ++i )
			{
				*dst++ = *src;
				*dst++ = *src;
				*dst++ = *src;
				++src;
				*dst++ = *src++;
			}
		}

		m_format = format;

		return true;
	}
	return false;
}

bool HostBitmap::CheckForMatch( const BitmapDimensions& bd, bool checkDimensions, bool& alphaConvert, const char* log )
{
	if( !IsValid() )
	{
		CCP_LOGWARN( "%s: invalid source or destination", log );
		return false;
	}

	if( checkDimensions &&
		( bd.GetWidth()	!= m_width || bd.GetHeight()	!= m_height ) )
	{
		CCP_LOGWARN( "%s: incompatible size", log );
		return false;
	}

	if( bd.GetTrueMipCount() != GetTrueMipCount() )
	{
		CCP_LOGWARN( "%s: miplevels mismatch, data may be truncated", log );
	}

	const bool formatMatch = bd.GetFormat() == m_format;
	alphaConvert  = bd.GetFormat() == PIXEL_FORMAT_B8G8R8X8_UNORM && m_format == PIXEL_FORMAT_B8G8R8A8_UNORM;

	if( !formatMatch && !alphaConvert )
	{
		CCP_LOGWARN( "%s: incompatible size/pixelformat", log );
		return false;
	}

	return true;
}

const char* HostBitmap::GetMipRawData( unsigned level, uint32_t arrayIndex ) const
{
	if( !IsValid() || level >= GetTrueMipCount() )
	{
		return nullptr;
	}
	if( arrayIndex >= GetArraySize() )
	{
		return nullptr;
	}
	unsigned offset = 0;
	if( IsCompressedFormat( m_format ) )
	{
		for( unsigned i = 0; i != level; ++i )
		{
			offset += GetMipHeight( i ) / 4 * GetMipPitch( i ) * ( std::max( m_volumeDepth >> i, 1u ) );
		}
	}
	else
	{
		for( unsigned i = 0; i != level; ++i )
		{
			offset += GetMipHeight( i ) * GetMipPitch( i ) * ( std::max( m_volumeDepth >> i, 1u ) );
		}
	}

	if( arrayIndex > 0 )
	{
		unsigned faceSize = (unsigned int)( m_data.size() / GetArraySize() ); // is it safe to assume this?
		offset += faceSize * arrayIndex;
	}

	return GetRawData() + offset;
}

char* HostBitmap::GetMipRawData( unsigned level, uint32_t arrayIndex )
{
	return const_cast<char*>( const_cast<const HostBitmap*>(this)->GetMipRawData( level, arrayIndex ) );	
}

unsigned HostBitmap::GetPitch() const
{
	if( !IsValid() )
	{
		return 0;
	}
	if( IsCompressed() )
	{
		return m_width * GetBlockByteSize( m_format ) / 4;
	}

	return m_width * GetBytesPerPixel( m_format );
}

const char* HostBitmap::GetRawData() const
{
	return IsValid() ? m_data.get() : nullptr;
}

char* HostBitmap::GetRawData()
{
	return IsValid() ? m_data.get() : nullptr;
}

const char* HostBitmap::GetRawData( unsigned x, unsigned y ) const
{
	return const_cast<HostBitmap*>(this)->GetRawData( x, y );
}

char* HostBitmap::GetRawData( unsigned x, unsigned y )
{
	if( !IsValid() )
	{
		return nullptr;
	}

	CCP_ASSERT( x < m_width );
	CCP_ASSERT( y < m_height );

	if( IsCompressed() )
	{
		return m_data.get() + y * GetPitch() + x * GetBlockByteSize( m_format ) / 4;
	}

	return m_data.get() + y * GetPitch() + x * GetBytesPerPixel( m_format );
}

size_t HostBitmap::GetRawDataSize() const
{
	return m_data.size();
}

size_t HostBitmap::GetArrayElementSize() const
{
	return m_data.size() / m_arraySize;
}

/// --------------------------------------------------
/// Description:
///   Take the pixels in the sub-block (margin, margin)...(width-margin,height-margin) and copy their
///   values into the border pixels to get a clamping effect.
/// --------------------------------------------------
bool HostBitmap::PopulateMargin( unsigned margin )
{
	if( !IsValid() || IsCompressed() || m_mipCount != 1 || 2 * margin >= GetWidth() || 2 * margin >= GetHeight() || GetType() != TEX_TYPE_2D || m_arraySize > 1 )
	{
		return false;
	}

	const unsigned bytesPerPixel  = GetBytesPerPixel( GetFormat() );
	const unsigned bytesPerMargin = bytesPerPixel * margin;


	const unsigned width  = GetWidth()  - 2 * margin;
	const unsigned height = GetHeight() - 2 * margin;
	
	//top margin
	const char* src = GetRawData( margin, margin );
	for( unsigned i = 0; i != margin; ++i )
	{
		char* dst = GetRawData( margin, i );
		memcpy( dst, src, bytesPerPixel * width );
	}

	//bottom margin
	src = GetRawData( margin, height + margin - 1 );
	for( unsigned i = 0; i != margin; ++i ) 
	{
		char* dst = GetRawData( margin, height + margin + i );
		memcpy( dst, src, bytesPerPixel * width );
	}
		
	for( unsigned y = 0; y != height; ++y )
	{
		//left margin
		src = GetRawData( margin, y + margin );
		char* dst = GetRawData( 0, y + margin );
		for( unsigned i = 0; i != margin; ++i ) 
		{
			for( unsigned j = 0; j != bytesPerPixel; ++j )
			{
				dst[i * bytesPerPixel + j] = src[j];
			}
		}

		//right margin
		src += ( width - 1 ) * bytesPerPixel;
		dst += width * bytesPerPixel + bytesPerMargin;
		for( unsigned i = 0; i != margin; ++i ) 
		{
			for( unsigned j = 0; j != bytesPerPixel; ++j )
			{
				dst[i * bytesPerPixel + j] = src[j];
			}
		}
	}
	
	return true;
}

bool HostBitmap::CopyChannel( HostBitmap *source, unsigned srcChannel, unsigned dstChannel )
{
	if( !IsValid() || IsCompressed() || !source->IsValid() || source->IsCompressed() )
	{
		CCP_LOGWARN( "HostBitmap.CopyChannel: Need a valid uncompressed bitmap" );
		return false;
	}

	if( m_type != source->m_type || m_arraySize != source->m_arraySize || m_mipCount != source->m_mipCount || m_width != source->m_width || m_height != source->m_height )
	{
		CCP_LOGWARN( "HostBitmap.CopyChannel: Bitmaps need same type and dimensions" );
		return false;
	}

	unsigned dstBPP = GetBytesPerPixel( GetFormat() );
	unsigned srcBPP = GetBytesPerPixel( source->GetFormat() );

	if( dstChannel >= dstBPP )
	{
		CCP_LOGWARN( "HostBitmap.CopyChannel: Destination channel out of range" );
		return false;
	}

	if( srcChannel >= srcBPP )
	{
		CCP_LOGWARN( "HostBitmap.CopyChannel: Source channel out of range" );
		return false;
	}

	if( source == this && srcChannel == dstChannel )
	{
		return true;
	}

	size_t dstSize = GetRawDataSize();
	size_t srcSize = source->GetRawDataSize();

	const char *src = source->GetRawData();
	char *dst = GetRawData();

	for( unsigned srcPos = srcChannel, dstPos = dstChannel; dstPos < dstSize && srcPos < srcSize; srcPos += srcBPP, dstPos += dstBPP )
	{
		dst[dstPos] = src[srcPos];
	}

	return true;
}

bool HostBitmap::Downsample2x2()
{
	if( !IsValid() || ( m_width & 1 ) || ( m_height & 1 ) || ( m_type != TEX_TYPE_2D && m_type != TEX_TYPE_CUBE ) )
	{
		CCP_LOGWARN( "Downsample2x2 only works with valid, even sized bitmaps" );
		return false;
	}

	auto format = GetFormat();
	switch( format )
	{
	case PIXEL_FORMAT_R8G8B8A8_TYPELESS:
	case PIXEL_FORMAT_R8G8B8A8_UNORM:
	case PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:
	case PIXEL_FORMAT_R8G8B8A8_UINT:
	case PIXEL_FORMAT_R8G8B8A8_SNORM:
	case PIXEL_FORMAT_R8G8B8A8_SINT:
	case PIXEL_FORMAT_R8G8_TYPELESS:
	case PIXEL_FORMAT_R8G8_UNORM:
	case PIXEL_FORMAT_R8G8_UINT:
	case PIXEL_FORMAT_R8G8_SNORM:
	case PIXEL_FORMAT_R8G8_SINT:
	case PIXEL_FORMAT_R8_TYPELESS:
	case PIXEL_FORMAT_R8_UNORM:
	case PIXEL_FORMAT_R8_UINT:
	case PIXEL_FORMAT_R8_SNORM:
	case PIXEL_FORMAT_R8_SINT:
	case PIXEL_FORMAT_A8_UNORM:
	case PIXEL_FORMAT_B8G8R8A8_UNORM:
	case PIXEL_FORMAT_B8G8R8X8_UNORM:
	case PIXEL_FORMAT_B8G8R8A8_TYPELESS:
	case PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
	case PIXEL_FORMAT_B8G8R8X8_TYPELESS:
	case PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB:
		break;
	default:
		CCP_LOGWARN( "Downsample2x2 does not support this pixel format" );
		return false;
	}

	const uint8_t* src = (const uint8_t*)m_data.get();
		  uint8_t* dst = (uint8_t*)m_data.get();
	const unsigned bpp = GetBytesPerPixel( m_format );

	unsigned newMipCount = std::max( m_mipCount - 1, 1u );

	for( unsigned l = 0; l < m_arraySize; l++ )
	{
		unsigned curWidth = m_width, curHeight = m_height;
		for( unsigned k = 0; k < newMipCount; k++, curWidth /= 2, curHeight /= 2 )
		{
			for( unsigned j = 0; j != curHeight/2; ++j )
			{
				for( unsigned i = 0; i != curWidth/2; ++i )
				{
					for( unsigned byte = 0; byte < bpp; ++byte, ++src, ++dst )
					{
						unsigned sum = src[0] + src[bpp] + src[curWidth*bpp] + src[curWidth*bpp + bpp];
						*dst = (uint8_t)( sum / 4 );
					}
					src += bpp;
				}
				src += curWidth * bpp;
			}
		}

		// Skip lowest mip level, but only if we had any mips to begin with
		if( m_mipCount != 1 )
		{
			src += curHeight * curWidth * bpp;
		}
	}

	m_mipCount = newMipCount;
	m_width /= 2;
	m_height /= 2;

	uint32_t mipCount = newMipCount;
	uint32_t width = m_width;
	uint32_t height = m_height;

	size_t size = 0;
	while( mipCount-- )
	{
		size += width * height * bpp;
		width = std::max( width / 2u, 1u );
		height = std::max( height / 2u, 1u );
	}

	m_data.resize( "HostBitmap::m_data", size * m_arraySize );

	if( m_data.empty() )
	{
		Destroy();
		return false;
	}
	
	return true;
}

bool HostBitmap::Crop( unsigned left, unsigned top, unsigned right, unsigned bottom )
{
	if( !IsValid() || m_mipCount != 1 || m_type != TEX_TYPE_2D || m_arraySize > 1 || IsCompressedFormat( m_format ) )
	{
		CCP_LOGWARN( "Crop only works with valid, single miplevel 2D bitmaps in uncompressed format" );
		return false;
	}

 	left = std::min( left, m_width );
	right = std::min( right, m_width );
	top = std::min( top, m_height );
	bottom = std::min( bottom, m_height );

	if( left >= right || top >= bottom )
	{
		Destroy();
		return true;
	}

	const unsigned bpp = GetBytesPerPixel( m_format );
	const unsigned srcStride = m_width * bpp;
	const unsigned dstStride = ( right - left ) * bpp;

	const uint8_t* src = (const uint8_t*)m_data.get() + left * bpp + srcStride * top;
		  uint8_t* dst = (uint8_t*)m_data.get();

	for( unsigned j = top; j != bottom; ++j )
	{
		memmove( dst, src, dstStride );
		dst += dstStride;
		src += srcStride;
	}
	m_width = right - left;
	m_height = bottom - top;
	m_data.resize( "HostBitmap::m_data", m_width * m_height * bpp );
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Rotates a face clockwise a multiple of 90 degrees
// --------------------------------------------------------------------------------
bool HostBitmap::RotateFaceClockwise( unsigned face, unsigned times )
{
	if( !IsValid() )
	{
		return false;
	}

	if( ( GetType() != TEX_TYPE_2D && GetType() != TEX_TYPE_CUBE ) || GetMipCount() > 1 )
	{
		CCP_LOGERR( "HostBitmap.RotateFaceClockwise requires 2D/CUBE bitmap with a single mip level" );
		return false;
	}

	if( face >= GetArraySize() )
	{
		CCP_LOGERR( "HostBitmap.RotateFaceClockwise: index out of range" );
		return false;
	}

	if( GetWidth() != GetHeight() )
	{
		CCP_LOGERR( "HostBitmap.RotateFaceClockwise: width must be equal to height" );
		return false;
	}
	
	if( IsCompressedFormat( GetFormat() ) )
	{
		CCP_LOGERR( "HostBitmap.RotateFaceClockwise: don't support compressed images" );
		return false;
	}

	times = times % 4;
	if( !times )
	{
		return true;
	}

	unsigned size = GetWidth();
	unsigned pitch = GetPitch();
	unsigned bpp = GetBytesPerPixel( m_format );

	CcpMallocBuffer newData( "HostBitmap::m_data", GetRawDataSize() );
	if( newData.empty() )
	{
		CCP_LOGERR( "HostBitmap.RotateFaceClockwise: out of memory" );
		return false;
	}

	const char *srcRaw = GetRawData();
	char *dstRaw = newData.get();

	memcpy( dstRaw, srcRaw, GetRawDataSize() );

	unsigned offset = pitch * size * face;
	const char *srcOffset = srcRaw + offset;
	char *dstOffset = dstRaw + offset;

	for( unsigned oldX = 0; oldX < size; oldX++ )
	{
		for( unsigned oldY = 0; oldY < size; oldY++ )
		{
			unsigned newX = oldX, newY = oldY;
			for( unsigned i = 0; i < times; i++ )
			{
				unsigned y = newX;
				newX = size - 1 - newY;
				newY = y;
			}

			const char *src = srcOffset + ( oldX + oldY * size ) * bpp;
			char *dst = dstOffset + ( newX + newY * size ) * bpp;
			for( unsigned k = 0; k < bpp; ++k )
			{
				dst[k] = src[k];
			}
		}
	}

	std::swap( m_data, newData );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Convert a 2D 3x4 crossmap to a cubemap
// --------------------------------------------------------------------------------
bool HostBitmap::ConvertCrossmapToCubemap()
{
	if( !IsValid() )
	{
		return false;
	}

	if( GetType() != TEX_TYPE_2D || m_arraySize > 1 )
	{
		CCP_LOGERR( "HostBitmap.ConvertCrossmapToCubemap requires a 2D bitmap" );
		return false;
	}
	
	if( GetMipCount() != 1 )
	{
		CCP_LOGERR( "HostBitmap.ConvertCrossmapToCubemap: Bitmap has mips. Use DropMipMaps() first." );
		return false;
	}

	// only support a single type crossmap for now
	if( GetWidth() % 3 != 0 || GetHeight() % 4 != 0 || GetWidth() / 3 != GetHeight() / 4 )
	{
		CCP_LOGERR( "HostBitmap.ConvertCrossmapToCubemap: source image does not represent a 3:4 crossmap!" );
		return false;
	}
	
	if( IsCompressedFormat( GetFormat() ) )
	{
		CCP_LOGERR( "HostBitmap.ConvertCrossmapToCubemap: don't support compressed images" );
		return false;
	}

	unsigned cubeSize = GetWidth() / 3;

	unsigned srcPitch = GetPitch();
	unsigned dstPitch = cubeSize * GetBytesPerPixel( m_format );

	unsigned faceOffsets[] =
	{
		srcPitch * cubeSize + srcPitch / 3 * 2,
		srcPitch * cubeSize,
		srcPitch / 3,
		srcPitch * cubeSize * 2 + srcPitch / 3,
		srcPitch * cubeSize + srcPitch / 3,
		srcPitch * cubeSize * 3 + srcPitch / 3,
	};

	CcpMallocBuffer newData( "HostBitmap::m_data", dstPitch * cubeSize * 6 );
	if( newData.empty() )
	{
		CCP_LOGERR( "HostBitmap.ConvertCrossmapToCubemap: out of memory" );
		return false;
	}

	const char *srcRaw = GetRawData();
	char *dstRaw = newData.get();

	for( unsigned face = 0; face < 6; face++ )
	{
		const char *srcOffset = srcRaw + faceOffsets[face];
		char *dstOffset = dstRaw + face * cubeSize * dstPitch;

		for( unsigned line = 0; line < cubeSize; ++line )
		{
			const char* srcLine = srcOffset + line * srcPitch;
			char* destLine = dstOffset + line * dstPitch;
			memcpy( destLine, srcLine, dstPitch );
		}
	}

	std::swap( m_data, newData );

	m_type = TEX_TYPE_CUBE;
	m_width = m_height = cubeSize;
	m_arraySize = 6;

	RotateFaceClockwise( 5, 2 );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Special function to convert a 2d representation of a 3d texture into a real
//   3d texture. photoshop dds plugin cannot do this properly...
// --------------------------------------------------------------------------------
bool HostBitmap::ConvertToVolume()
{
	if( !IsValid() )
	{
		return false;
	}

	if( GetType() != TEX_TYPE_2D || m_arraySize > 1 )
	{
		CCP_LOGERR( "HostBitmap.ConvertToVolume requires 2D bitmap" );
		return false;
	}

	unsigned cubeSize = GetHeight();
	// only support cubic volume textures by now
	if( cubeSize * cubeSize < GetWidth() )
	{
		CCP_LOGERR( "HostBitmap.ConvertToVolume: source image does not represent a cubic volume texture!" );
		return false;
	}

	if( IsCompressedFormat( GetFormat() ) )
	{
		if( ( cubeSize % 4 ) != 0 )
		{
			CCP_LOGERR( "HostBitmap.ConvertToVolume: %i is not a valid size for compressed texture", cubeSize );
			return false;
		}
	}

	cubeSize = (unsigned)sqrtf(float(GetWidth()));
	unsigned rowPitch = GetPitch() / GetWidth() * cubeSize;
	unsigned slicePitch = rowPitch * cubeSize;
	unsigned volumeSize = slicePitch * cubeSize;

	CcpMallocBuffer newData( "HostBitmap::m_data", volumeSize );
	if( newData.empty() )
	{
		CCP_LOGERR( "HostBitmap.ConvertToVolume: out of memory" );
		return false;
	}

	const unsigned sourceOffset = ( GetHeight() - cubeSize ) * GetPitch();
	for( unsigned int slice = 0; slice < cubeSize; ++slice )
	{
		const char* srcSlice = GetRawData() + sourceOffset + ( slice * GetPitch() / cubeSize );
		char* destSlice = newData.get() + slice * slicePitch;
		for( unsigned int line = 0; line < cubeSize; ++line)
		{
			const char* srcLine = ( line * GetPitch() ) + srcSlice;
			char* destLine = ( line * rowPitch ) + destSlice;
			// copy line
			memcpy( destLine, srcLine, rowPitch );
		}
	}

	std::swap( m_data, newData );

	m_type = TEX_TYPE_3D;
	m_mipCount = 1;
	m_width = m_height = m_volumeDepth = cubeSize;

	return true;
}


// --------------------------------------------------------------------------------------
// Description:
//   Generates all mipmaps for the image. Only 2D images and formats with 8bits per 
//   channel are supported.
// Arguments:
//   format - New image format
// Return Value:
//   true When conversion was successful
//   false Otherwise
// --------------------------------------------------------------------------------------
bool HostBitmap::GenerateMipMaps( unsigned levels )
{
	if( ( m_type != TEX_TYPE_2D && m_type != TEX_TYPE_CUBE ) || m_mipCount > 1 )
	{
		return false;
	}

	auto format = GetFormat();
	switch( format )
	{
	case PIXEL_FORMAT_R8G8B8A8_TYPELESS:
	case PIXEL_FORMAT_R8G8B8A8_UNORM:
	case PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:
	case PIXEL_FORMAT_R8G8B8A8_UINT:
	case PIXEL_FORMAT_R8G8B8A8_SNORM:
	case PIXEL_FORMAT_R8G8B8A8_SINT:
	case PIXEL_FORMAT_R8G8_TYPELESS:
	case PIXEL_FORMAT_R8G8_UNORM:
	case PIXEL_FORMAT_R8G8_UINT:
	case PIXEL_FORMAT_R8G8_SNORM:
	case PIXEL_FORMAT_R8G8_SINT:
	case PIXEL_FORMAT_R8_TYPELESS:
	case PIXEL_FORMAT_R8_UNORM:
	case PIXEL_FORMAT_R8_UINT:
	case PIXEL_FORMAT_R8_SNORM:
	case PIXEL_FORMAT_R8_SINT:
	case PIXEL_FORMAT_A8_UNORM:
	case PIXEL_FORMAT_B8G8R8A8_UNORM:
	case PIXEL_FORMAT_B8G8R8X8_UNORM:
	case PIXEL_FORMAT_B8G8R8A8_TYPELESS:
	case PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:
	case PIXEL_FORMAT_B8G8R8X8_TYPELESS:
	case PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB:
		break;
	default:
		return false;
	}

	m_mipCount = 0;
	uint32_t mipCount = GetTrueMipCount();
	if( levels )
	{
		if( levels > mipCount )
		{
			return false;
		}

		mipCount = levels;
	}
	else
	{
		levels = mipCount;
	}

	uint32_t width = m_width;
	uint32_t height = m_height;

	size_t size = 0;
	while( mipCount-- )
	{
		size += width * height * GetBytesPerPixel( format );
		width  = std::max( width  / 2u, 1u );
		height = std::max( height / 2u, 1u );
	}

	size_t originalSize = m_data.size();

	m_data.resize( "HostBitmap::m_data", size * GetArraySize() );
	if( m_data.empty() )
	{
		return false;
	}

	for( int32_t j = GetArraySize() - 1; j >= 0; --j )
	{
		width = m_width;
		height = m_height;

		size_t topSize = width * height * GetBytesPerPixel( m_format );
		memcpy( m_data.get() + m_data.size() / GetArraySize() * j, m_data.get() + originalSize / GetArraySize() * j, topSize );

		for( unsigned i = 0; i + 1 < levels; ++i )
		{
			if( !GenerateMipLevel( reinterpret_cast<uint8_t*>( GetMipRawData( i, j ) ), width, height, reinterpret_cast<uint8_t*>( GetMipRawData( i + 1, j ) ) ) )
			{
				return false;
			}
			width = std::max( width / 2, 1u );
			height = std::max( height / 2, 1u );
		}
	}
	m_mipCount = levels;

	return true;
}

bool HostBitmap::GenerateMipLevel( uint8_t* source, unsigned width, unsigned height, uint8_t* destination )
{
	unsigned bpp = GetBytesPerPixel( m_format );
	unsigned dstWidth = std::max( width / 2, 1u );
	unsigned dstHeight = std::max( height / 2, 1u );
	unsigned srcStride = width * bpp;

	unsigned vertStep = height > 1 ? srcStride : 0;
	unsigned horizStep = width > 1 ? bpp : 0;

	uint8_t* src00 = source;
	uint8_t* src01 = source + vertStep;

	uint8_t* bottom = source + ( height - 1 ) * srcStride;

	for( unsigned i = 0; i < dstHeight; ++i )
	{
		uint8_t* right0 = src00 + ( width - 1 ) * bpp;
		uint8_t* right1 = src01 + ( width - 1 ) * bpp;
		for( unsigned j = 0; j < dstWidth; ++j )
		{
			uint8_t* src10 = std::min( src00 + horizStep, right0 );
			uint8_t* src11 = std::min( src01 + horizStep, right1 );
			for( unsigned k = 0; k < bpp; ++k )
			{
				*destination++ = ( unsigned( *src00++ ) + unsigned( *src01++ ) + unsigned( *src10++ ) + unsigned( *src11++ ) ) >> 2;
			}
			src00 += horizStep;
			src01 += horizStep;
		}
		src00 = std::min( right0 + bpp + vertStep, bottom );
		src01 = std::min( right1 + bpp + vertStep, bottom );
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes mipmaps from the image. Only 2D images and formats with 8bits per 
//   channel are supported.
// Return Value:
//   Will always return true, unless the bitmap is invalid
// --------------------------------------------------------------------------------------
bool HostBitmap::DropMipMaps()
{
	if( !IsValid() )
	{
		CCP_LOGERR( "HostBitmap.DropMipMaps: bitmap is not valid" );
		return false;
	}

	if( m_mipCount == 1 )
	{
		return true;
	}

	size_t size = GetMipRawData( 1, 0 ) - GetRawData();
	for( unsigned i = 1; i < m_arraySize; i++ ) // index 0 is already at the right place, skip it
	{
		memcpy( GetRawData() + size * i, GetMipRawData( 0, i ), size );
	}
	m_data.resize( "HostBitmap::m_data", size * GetArraySize() );
	m_mipCount = 1;

	return true;
}

bool HostBitmap::GetAverageColor(float &r, float &g, float &b, float &a) {
	if( !IsValid() )
	{
		CCP_LOGERR( "GetAverageColor: bitmap %s is not valid" );
		return false;
	}

	if( GetType() == TEX_TYPE_2D && GetArraySize() < 2 )
	{
		auto format = GetFormat();
		if( format != PIXEL_FORMAT_B8G8R8X8_UNORM &&
			format != PIXEL_FORMAT_B8G8R8A8_UNORM &&
			format != PIXEL_FORMAT_BC1_UNORM &&
			format != PIXEL_FORMAT_BC3_UNORM ) {
			return false;
		}

		const uint32_t mipLevel = GetMipCount() - 1;

		const uint32_t width = GetMipWidth( mipLevel );
		const uint32_t height = GetMipHeight( mipLevel );

		if( width == 0 || height == 0 )
		{
			return false;
		}

		const uint32_t pitch = GetMipPitch( mipLevel );
		const char* data = GetMipRawData( mipLevel );

        uint32_t yStep = std::min( height, std::max( 1u, uint32_t( sqrt( float( height ) ) ) ) );
		uint32_t xStep = std::min( width, std::max( 1u, uint32_t( sqrt( float( width ) ) ) ) );
		
		// get the sample count, rounded up
		uint32_t xSampleCount = uint32_t( float( width ) / float( xStep ) + 0.5f );
		uint32_t ySampleCount = uint32_t( float( height ) / float( yStep ) + 0.5f );

		// offset the grid by half of the remainder of the step
		uint32_t xOffset = (width % xStep) / 2;
		uint32_t yOffset = (height % yStep) / 2;

		if( xOffset == 0 )
		{
			xOffset = xStep / 2;
		}
		if( yOffset == 0 )
		{
			yOffset = yStep / 2;
		}

		uint32_t rChannel = 0, bChannel = 0, gChannel = 0, aChannel = 0;

		std::function<uint32_t( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* data )> GetPixel;

		// Setup the correct get pixel function
		switch( format ) 
		{
		case PIXEL_FORMAT_B8G8R8A8_UNORM:
			GetPixel = []( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* data ) { return ImageUtility::GetPixelColor_BGRA( x, y, pitch, data ); };
			break;
		case PIXEL_FORMAT_B8G8R8X8_UNORM:
			GetPixel = []( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* data ) { return ImageUtility::GetPixelColor_BGRX( x, y, pitch, data ); };
			break;
		case PIXEL_FORMAT_BC1_UNORM:
			GetPixel = []( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* data ) { return ImageUtility::GetPixelColor_BC1( x, y, width, pitch, data ); };
			break;
		default:			
			GetPixel = []( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* data ) { return ImageUtility::GetPixelColor_BC3( x, y, width, pitch, data ); };
			break;
		}

		// Go over the image and select the pixels every horizontal/vertical step
		for( uint32_t x = 0; x < xSampleCount; ++x )
		{
			for( uint32_t y = 0; y < ySampleCount; ++y )
			{
				if( x * xStep + xOffset >= width ) 
				{
					continue;
				}
				if( y * yStep + yOffset >= height) 
				{
					continue;
				}
				uint32_t pixelValue = GetPixel( x * xStep + xOffset, y * yStep + yOffset, width, pitch, data );
				rChannel += (pixelValue & 0x00ff0000) >> 16;
				gChannel += (pixelValue & 0x0000ff00) >> 8;
				bChannel += (pixelValue & 0x000000ff);
				aChannel += (pixelValue & 0xff000000) >> 24;
			}
		}
		
		float multiplier = 1.0f / float( xSampleCount * ySampleCount ) / 255.f;
		a = float(aChannel * multiplier);
		r = float(rChannel * multiplier);
		g = float(gChannel * multiplier);
		b = float(bChannel * multiplier);

		return true;
	}
	return false;
}

bool HostBitmap::GetPixel( uint32_t x, uint32_t y, float& r, float& g, float& b, float& a ) const
{
	if( !IsValid() )
	{
		CCP_LOGERR( "GetPixel: bitmap is not valid" );
		return false;
	}

	if( GetType() == TEX_TYPE_2D )
	{
		auto format = GetFormat();
		if( format != PIXEL_FORMAT_B8G8R8X8_UNORM &&
			format != PIXEL_FORMAT_B8G8R8A8_UNORM &&
			format != PIXEL_FORMAT_BC1_UNORM &&
			format != PIXEL_FORMAT_BC3_UNORM ) {
			return false;
		}

		const uint32_t width = GetWidth();
		const uint32_t height = GetHeight();

		if( x > width || y > height ) {
			CCP_LOGERR( "GetPixel: pixel index out of range. Requested pixel (%d, %d), dimensions (%d, %d)", x, y, width, height );
			return false;
		}
		const uint32_t pitch = GetPitch();
		const char* data = GetRawData();
		unsigned int pixelValue = 0;
		switch( format ) {
		case PIXEL_FORMAT_R8_UNORM:
			pixelValue = ImageUtility::GetPixelColor_R( x, y, pitch, data );
			break;
		case PIXEL_FORMAT_B8G8R8A8_UNORM:
			pixelValue = ImageUtility::GetPixelColor_BGRA( x, y, pitch, data );
			break;
		case PIXEL_FORMAT_B8G8R8X8_UNORM:
			pixelValue = ImageUtility::GetPixelColor_BGRX( x, y, pitch, data );
			break;
		case PIXEL_FORMAT_BC1_UNORM:
			pixelValue = ImageUtility::GetPixelColor_BC1( x, y, width, pitch, data );
			break;
		case PIXEL_FORMAT_BC3_UNORM:
			pixelValue = ImageUtility::GetPixelColor_BC3( x, y, width, pitch, data );
			break;
		default:
			break;
		}
		r = float( (pixelValue & 0x00ff0000) >> 16 ) / 255.f;
		g = float( (pixelValue & 0x0000ff00) >> 8 ) / 255.f;
		b = float( (pixelValue & 0x000000ff)  ) / 255.f;
		a = float( (pixelValue & 0xff000000) >> 24 ) / 255.f;
		return true;
	}
	return false;
}

}
