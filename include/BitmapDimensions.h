// Copyright © 2024 CCP ehf.

#pragma once

#include <cstdint>
#include <algorithm>
#include "PixelFormat.h"
#include "TextureType.h"

namespace ImageIO
{

// -------------------------------------------------------------
// Description:
// Helper to keep track of variables that usually go together,
// and functions to do some repeated math around them (computing
// size of a given miplevel, etc).
// -------------------------------------------------------------
struct BitmapDimensions
{
	BitmapDimensions();

	BitmapDimensions( uint32_t width,
					  uint32_t height,
					  uint32_t mipCount,
					  PixelFormat format );

	BitmapDimensions( TextureType type,
					  PixelFormat format,
					  uint32_t width,
					  uint32_t height,
					  uint32_t depth,
					  uint32_t mipCount );

	BitmapDimensions( TextureType type,
					  PixelFormat format,
					  uint32_t width,
					  uint32_t height,
					  uint32_t depth,
					  uint32_t mipCount,
					  uint32_t arraySize );

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDepth() const;
	PixelFormat GetFormat() const;
	uint32_t GetMipCount() const;
	uint32_t GetTrueMipCount() const;
	bool IsCompressed() const;
	uint32_t HasMipmap() const;

	uint32_t GetMipWidth( uint32_t level ) const;
	uint32_t GetMipHeight( uint32_t level ) const;
	uint32_t GetMipDepth( uint32_t level ) const;
	uint32_t GetMipPitch( uint32_t level ) const;
	uint32_t GetMipSize( uint32_t level ) const;

	uint32_t GetArraySize() const;

	// Number of rows to copy in a mip. For non-compressed formats this is the same as GetMipHeight.
	// For compressed formats it's that number / 4.
	uint32_t GetMipNumRows( uint32_t level ) const;

	TextureType GetType() const;

	bool operator==( const BitmapDimensions& other ) const;

protected:
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_volumeDepth;
	uint32_t m_mipCount;
	uint32_t m_arraySize;
	TextureType m_type;
	PixelFormat m_format;

	void Destroy()
	{
		// m_width	= m_height = m_volumeDepth = m_mipCount = 0;
		m_type = TEX_TYPE_INVALID;
		m_format = PIXEL_FORMAT_UNKNOWN;
	}
};

inline BitmapDimensions::BitmapDimensions() :
	m_width( 0 ),
	m_height( 0 ),
	m_volumeDepth( 0 ),
	m_mipCount( 0 ),
	m_arraySize( 1 ),
	m_type( TEX_TYPE_INVALID ),
	m_format( PIXEL_FORMAT_UNKNOWN )
{
}

inline BitmapDimensions::BitmapDimensions(
	uint32_t width,
	uint32_t height,
	uint32_t mipCount,
	PixelFormat format ) :
	m_width( width ),
	m_height( height ),
	m_volumeDepth( 1 ),
	m_mipCount( mipCount ),
	m_arraySize( 1 ),
	m_type( TEX_TYPE_2D ),
	m_format( format )
{
}

inline BitmapDimensions::BitmapDimensions(
	TextureType type,
	PixelFormat format,
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	uint32_t mipCount ) :
	m_width( width ),
	m_height( height ),
	m_volumeDepth( depth ),
	m_mipCount( mipCount ),
	m_arraySize( type == TEX_TYPE_CUBE ? 6 : 1 ),
	m_type( type ),
	m_format( format )
{
}

inline BitmapDimensions::BitmapDimensions(
	TextureType type,
	PixelFormat format,
	uint32_t width,
	uint32_t height,
	uint32_t depth,
	uint32_t mipCount,
	uint32_t arraySize ) :
	m_width( width ),
	m_height( height ),
	m_volumeDepth( depth ),
	m_mipCount( mipCount ),
	m_arraySize( arraySize ),
	m_type( type ),
	m_format( format )
{
}

inline uint32_t BitmapDimensions::GetWidth() const
{
	return m_width;
}

inline uint32_t BitmapDimensions::GetHeight() const
{
	return m_height;
}

inline uint32_t BitmapDimensions::GetDepth() const
{
	return m_volumeDepth;
}

inline PixelFormat BitmapDimensions::GetFormat() const
{
	return m_format;
}

inline uint32_t BitmapDimensions::GetMipCount() const
{
	return m_mipCount;
}

inline uint32_t BitmapDimensions::GetTrueMipCount() const
{
	if( m_mipCount > 0 )
	{
		return m_mipCount;
	}
	uint32_t size = std::max( m_width, m_height );
	uint32_t count = 0;
	while( size )
	{
		++count;
		size >>= 1;
	}
	return count;
}

inline bool BitmapDimensions::IsCompressed() const
{
	return IsCompressedFormat( m_format );
}

inline uint32_t BitmapDimensions::HasMipmap() const
{
	return m_mipCount != 1;
}

inline uint32_t BitmapDimensions::GetMipWidth( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return std::max( ( ( m_width >> level ) + 3u ) & ~3u, 4u );
	}

	return std::max( m_width >> level, 1u );
}

inline uint32_t BitmapDimensions::GetMipHeight( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return std::max( ( ( m_height >> level ) + 3u ) & ~3u, 4u );
	}

	return std::max( m_height >> level, 1u );
}

inline uint32_t BitmapDimensions::GetMipDepth( uint32_t level ) const
{
	if( m_type != TEX_TYPE_3D )
	{
		return 1;
	}

	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	return std::max( m_volumeDepth >> level, 1u );
}

inline uint32_t BitmapDimensions::GetMipPitch( uint32_t level ) const
{
	if( level >= GetTrueMipCount() )
	{
		return 0;
	}

	if( IsCompressed() )
	{
		return GetMipWidth( level ) / 4u * GetBlockByteSize( m_format );
	}

	return GetMipWidth( level ) * GetBytesPerPixel( m_format );
}

inline uint32_t BitmapDimensions::GetMipSize( uint32_t level ) const
{
	if( IsCompressed() )
	{
		return GetMipWidth( level ) * GetMipHeight( level ) * GetMipDepth( level ) / 16 * GetBlockByteSize( m_format );
	}

	return GetMipWidth( level ) * GetMipHeight( level ) * GetMipDepth( level ) * GetBytesPerPixel( m_format );
}

inline uint32_t BitmapDimensions::GetArraySize() const
{
	return m_arraySize;
}

inline uint32_t BitmapDimensions::GetMipNumRows( uint32_t level ) const
{
	return IsCompressed() ? GetMipHeight( level ) / 4 : GetMipHeight( level );
}

inline TextureType BitmapDimensions::GetType() const
{
	return m_type;
}

inline bool BitmapDimensions::operator==( const BitmapDimensions& other ) const
{
	return m_width == other.m_width &&
		m_height == other.m_height &&
		m_volumeDepth == other.m_volumeDepth &&
		m_mipCount == other.m_mipCount &&
		m_arraySize == other.m_arraySize &&
		m_type == other.m_type &&
		m_format == other.m_format;
}

} // namespace ImageIO