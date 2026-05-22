// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "NanoVdbSupport.h"
#include "HostBitmap.h"

#include "nanovdb/util/IO.h"


namespace
{
bool AdjustBounds( const char* vdbPath, const std::vector<ImageIO::RasterizeGridInfo>& grids, nanovdb::CoordBBox& totalBounds, std::vector<std::pair<float, float>>& ranges )
{
	std::vector<nanovdb::GridHandle<nanovdb::HostBuffer>> handles;
	try
	{
		handles = nanovdb::io::readGrids( vdbPath );
	}
	catch( const std::runtime_error& )
	{
		return false;
	}

	for( size_t i = 0; i < grids.size(); ++i )
	{
		auto gridNumber = grids[i].index;
		if( gridNumber >= handles.size() )
		{
			return false;
		}
		auto* grid = handles[gridNumber].grid<float>( 0 );
		if( !grid )
		{
			return false;
		}
		if( grid->isEmpty() )
		{
			return true;
		}
		auto bBox = grid->indexBBox();
		totalBounds.expand( bBox.max() );
		totalBounds.expand( bBox.min() );

		auto acc = grid->tree().getAccessor();
		auto& minD = ranges[i].first;
		auto& maxD = ranges[i].second;
		for( auto it = bBox.begin(); it; ++it )
		{
			auto density = acc.getValue( *it );
			minD = std::min( minD, density );
			maxD = std::max( maxD, density );
		}
	}
	return true;
}

struct VectorOutStream: public ICcpStream
{
	VectorOutStream( std::vector<uint8_t>& vec ) :
		m_vec( vec )
	{
	}

	ptrdiff_t Read( void* dest, ptrdiff_t count )
	{
		return 0;
	}

	ptrdiff_t Write( const void* source, size_t count )
	{
		size_t offset = m_vec.size();
		m_vec.resize( offset + count );
		if( m_vec.size() != offset + count )
		{
			return 0;
		}
		memcpy( m_vec.data() + offset, source, count );
		return count;
	}

	ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin method )
	{
		return 0;
	}

	ptrdiff_t GetPosition()
	{
		return ptrdiff_t( m_vec.size() );
	}

	ptrdiff_t GetSize()
	{
		return 0;
	}

private:
	std::vector<uint8_t>& m_vec;
};

struct MemInStream : public ICcpStream
{
	MemInStream( const void* data, size_t size ) :
		m_data( static_cast<const uint8_t*>( data ) ),
		m_position( static_cast<const uint8_t*>( data ) ),
		m_size( size )
	{
	}

	ptrdiff_t Read( void* dest, ptrdiff_t count )
	{
		if( count < 0 )
		{
			count = m_size - ( m_position - m_data );
		}
		count = std::min( count, ptrdiff_t( m_size - ( m_position - m_data ) ) );
		memcpy( dest, m_position, count );
		m_position += count;
		return count;
	}

	ptrdiff_t Write( const void* source, size_t count )
	{
		return 0;
	}

	ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin method )
	{
		switch( method )
		{
		case SO_BEGIN:
			m_position = std::min( m_data + m_size, m_data + std::max( distance, ptrdiff_t( 0 ) ) );
			break;
		case SO_CURRENT:
			m_position = std::min( std::max( m_position + distance, m_data ), m_data + m_size );
			break;
		case SO_END:
			m_position = std::min( std::max( m_position + m_size - distance, m_data ), m_data + m_size );
			break;
		default:
			return -1;
		}
		return GetPosition();
	}

	ptrdiff_t GetPosition()
	{
		return m_position - m_data;
	}

	ptrdiff_t GetSize()
	{
		return m_size;
	}

private:
	const uint8_t* m_data;
	const uint8_t* m_position;
	size_t m_size;
};

size_t WriteToVector( const void* buffer, size_t size, void* ctx )
{
	auto& dest = *static_cast<std::vector<uint8_t>*>( ctx );
	size_t offset = dest.size();
	dest.resize( offset + size );
	memcpy( dest.data() + offset, buffer, size );
	return size;
}

void DownsampleVolume( const ImageIO::HostBitmap& srcBitmap, ImageIO::HostBitmap& dstBitmap )
{
	// This function assumes dest bitmap is 1/2 the size of the source in all dimensions, and has R8 format
	uint32_t wd = dstBitmap.GetWidth();
	uint32_t hd = dstBitmap.GetHeight();
	uint32_t dd = dstBitmap.GetDepth();

	uint32_t ws = srcBitmap.GetWidth();
	uint32_t hs = srcBitmap.GetHeight();

	CcpMallocBuffer data;
	data.resize( "", wd * hd * dd );
	auto dst = reinterpret_cast<uint8_t*>( dstBitmap.GetRawData() );
	auto src = reinterpret_cast<const uint8_t*>( srcBitmap.GetRawData() );

	for( uint32_t k = 0; k < dd; ++k )
	{
		for( uint32_t j = 0; j < hd; ++j )
		{
			for( uint32_t i = 0; i < wd; ++i )
			{
				dst[i + j * wd + k * wd * hd] =
					( uint32_t( src[i * 2 + j * 2 * ws + k * 2 * ws * hs] ) +
					  uint32_t( src[( i * 2 + 1 ) + j * 2 * ws + k * 2 * ws * hs] ) +
					  uint32_t( src[i * 2 + ( j * 2 + 1 ) * ws + k * 2 * ws * hs] ) +
					  uint32_t( src[( i * 2 + 1 ) + ( j * 2 + 1 ) * ws + k * 2 * ws * hs] ) +

					  uint32_t( src[i * 2 + j * 2 * ws + ( k * 2 + 1 ) * ws * hs] ) +
					  uint32_t( src[( i * 2 + 1 ) + j * 2 * ws + ( k * 2 + 1 ) * ws * hs] ) +
					  uint32_t( src[i * 2 + ( j * 2 + 1 ) * ws + ( k * 2 + 1 ) * ws * hs] ) +
					  uint32_t( src[( i * 2 + 1 ) + ( j * 2 + 1 ) * ws + ( k * 2 + 1 ) * ws * hs] ) ) /
					8;
			}
		}
	}
}

}

namespace ImageIO
{

bool RasterizeNanoVDB( const char* vdbPath, uint32_t gridNumber, PixelFormat format, int32_t bpp, HostBitmap& bitmap )
{
	if( format != PIXEL_FORMAT_R8_UNORM && format != PIXEL_FORMAT_R32_FLOAT )
	{
		return false;
	}
	nanovdb::GridHandle<nanovdb::HostBuffer> handle;
	try
	{
		handle = nanovdb::io::readGrid( vdbPath, gridNumber );
	}
	catch( const std::runtime_error& )
	{
		return false;
	}

	auto* grid = handle.grid<float>( 0 );
	if( !grid )
	{
		return false;
	}

	auto bBox = grid->indexBBox();
	
	HostBitmap& bmp = bitmap;
	bmp.CreateVolume( bBox.dim().x(), bBox.dim().y(), bBox.dim().z(), 1, format );
	if( !bmp.IsValid() )
	{
		return false;
	}
	auto acc = grid->tree().getAccessor();

	float minD = std::numeric_limits<float>::max();
	float maxD = -std::numeric_limits<float>::max();
	for( auto it = bBox.begin(); it; ++it )
	{
		auto density = acc.getValue( *it );
		minD = std::min( minD, density );
		maxD = std::max( maxD, density );
	}


	if( format == PIXEL_FORMAT_R8_UNORM )
	{
		if( bpp < 0 )
		{
			for( auto it = bBox.begin(); it; ++it )
			{
				auto pos = *it;
				auto density = acc.getValue( pos );
				pos -= bBox.min();

				uint8_t val = std::min( std::max( int32_t( ( density - minD ) / ( maxD - minD ) * 255 + 0.5f ), 0 ), 255 );
				if( val >= 128 )
				{
					val &= 0b11000000;
				}
				else if( val >= 64 )
				{
					val &= 0b1100000;
				}
				else if( val >= 32 )
				{
					val &= 0b110000;
				}
				else if( val >= 16 )
				{
					val &= 0b11000;
				}
				else if( val >= 8 )
				{
					val &= 0b1100;
				}
				else if( val >= 4 )
				{
					val &= 0b110;
				}
				else
				{
					val &= 0b10;
				}
				reinterpret_cast<uint8_t*>( bmp.GetRawData() )[pos.z() * bBox.dim().x() * bBox.dim().y() + pos.y() * bBox.dim().x() + pos.x()] = val;
			}
		}
		else
		{
			uint8_t mask = 0xff;
			float middle = 0.5f;
			if( bpp > 1 && bpp < 8 )
			{
				mask = ~( uint8_t( ( 1 << ( 8 - bpp ) ) - 1 ) );
				middle *= 1 << ( 8 - bpp );
			}

			for( auto it = bBox.begin(); it; ++it )
			{
				auto pos = *it;
				auto density = acc.getValue( pos );
				pos -= bBox.min();

				uint8_t val = mask & std::min( std::max( int32_t( ( density - minD ) / ( maxD - minD ) * 255 + middle ), 0 ), 255 );
				reinterpret_cast<uint8_t*>( bmp.GetRawData() )[pos.z() * bBox.dim().x() * bBox.dim().y() + pos.y() * bBox.dim().x() + pos.x()] = val;
			}
		}
	}
	else
	{
		for( auto it = bBox.begin(); it; ++it )
		{
			auto pos = *it;
			auto density = acc.getValue( pos );
			pos -= bBox.min();
			reinterpret_cast<float*>( bmp.GetRawData() )[pos.z() * bBox.dim().x() * bBox.dim().y() + pos.y() * bBox.dim().x() + pos.x()] = density;
		}
	}
	return true;
}

bool GetNanoVDBMetaData( const char* vdbPath, std::vector<NanoVDBGridMetadata>& metaData )
{
	std::vector<nanovdb::io::GridMetaData> md;
	try
	{
		md = nanovdb::io::readGridMetaData( vdbPath );
	}
	catch( const std::runtime_error& )
	{
		return false;
	}
	metaData.reserve( md.size() );
	for( auto& data : md )
	{
		NanoVDBGridMetadata out;
		out.name = data.gridName;
		out.width = data.indexBBox.dim().x();
		out.height = data.indexBBox.dim().y();
		out.depth = data.indexBBox.dim().z();
		out.type = data.gridType;
		metaData.push_back( out );
	}
	return true;
}


std::vector<uint8_t> NanoVDBToVTA( const std::vector<std::string>& vdbPaths, const std::vector<RasterizeGridInfo>& grids )
{
	nanovdb::CoordBBox totalBounds;
	std::vector<std::pair<float, float>> range;
	for( auto grid: grids )
	{
		range.push_back( { std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() } );
	}

	// Get min/max values for each grid in all frames and common bounding box for all grids/frames
	for( auto& vdbPath : vdbPaths )
	{
		if( !AdjustBounds( vdbPath.c_str(), grids, totalBounds, range ) )
		{
			return {};
		}
	}
	if( totalBounds.empty() )
	{
		return {};
	}

	std::vector<Vta::GridInfo> gridInfo;
	for( auto g : grids )
	{
		Vta::GridInfo grid;
		grid.width = totalBounds.dim().x();
		grid.height = totalBounds.dim().y();
		grid.depth = totalBounds.dim().z();
		grid.format = g.format;
		grid.encoding = g.encoding;
		gridInfo.push_back( grid );
	}

	std::vector<HostBitmap> bitmaps;
	bitmaps.resize( grids.size() );
	for( auto& bmp : bitmaps )
	{
		bmp.CreateVolume( totalBounds.dim().x(), totalBounds.dim().y(), totalBounds.dim().z(), 1, PIXEL_FORMAT_R8_UNORM );
		if( !bmp.IsValid() )
		{
			return {};
		}
	}

	Vta::FileWriter builder( gridInfo, uint32_t( vdbPaths.size() ) );

	uint32_t frameIndex = 0;
	for( auto& vdbPath : vdbPaths )
	{
		auto handles = nanovdb::io::readGrids( vdbPath );

		for( uint32_t gridIndex = 0; gridIndex < grids.size(); ++gridIndex )
		{
			auto grid = handles[grids[gridIndex].index].grid<float>( 0 );
			if( frameIndex == 0 )
			{
				builder.SetGridName( gridIndex, grid->gridName() );
			}

			auto& bmp = bitmaps[gridIndex];

			auto bBox = grid->indexBBox();
			auto acc = grid->tree().getAccessor();

			float minD = range[gridIndex].first;
			float maxD = range[gridIndex].second;

			for( auto it = totalBounds.begin(); it; ++it )
			{
				auto pos = *it;
				auto density = bBox.isInside( pos ) ? acc.getValue( pos ) : 0;
				pos -= totalBounds.min();


				uint8_t val = std::min( std::max( int32_t( ( density - minD ) / ( maxD - minD ) * 255 + 0.5f ), 0 ), 255 );
				reinterpret_cast<uint8_t*>( bmp.GetRawData() )[pos.z() * totalBounds.dim().x() * totalBounds.dim().y() + pos.y() * totalBounds.dim().x() + pos.x()] = val;
			}

				builder.AddFrame( gridIndex, frameIndex, bmp );
			}
		++frameIndex;
	}

	std::vector<uint8_t> output;
	VectorOutStream stream( output );
	builder.WriteData( stream );
	return output;
}

std::vector<uint8_t> DownsampleVTA( const void* source, size_t sourceSize )
{
	MemInStream in( source, sourceSize );
	Vta::FileReader reader;
	reader.SetStream( &in );

	std::vector<Vta::GridInfo> gridInfo;
	std::vector<Vta::FrameDecoder> decoders;
	std::vector<HostBitmap> bitmaps;
	bitmaps.resize( reader.GetGridCount() );
	for( uint32_t i = 0; i < reader.GetGridCount(); ++i )
	{
		auto info = reader.GetGridInfo( i );
		info.width = std::max( 1u, info.width / 2 );
		info.height = std::max( 1u, info.height / 2 );
		info.depth = std::max( 1u, info.depth / 2 );
		gridInfo.push_back( info );

		decoders.emplace_back( Vta::FrameDecoder( reader, i ) );

		bitmaps[i].CreateVolume( info.width, info.height, info.depth, 1, info.format );
	}

	Vta::FileWriter builder( gridInfo, reader.GetFrameCount() );
	
	for( uint32_t i = 0; i < reader.GetFrameCount(); ++i )
	{
		for( uint32_t j = 0; j < reader.GetGridCount(); ++j )
		{
			auto& src = decoders[j].GetFrameBitmap();
			DownsampleVolume( src, bitmaps[j] );
			builder.AddFrame( j, i, bitmaps[j] );
		}
	}

	std::vector<uint8_t> output;
	VectorOutStream stream( output );
	builder.WriteData( stream );
	return output;
}


}