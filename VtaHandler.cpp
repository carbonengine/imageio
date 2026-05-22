// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "VtaHandler.h"
#include "HostBitmap.h"
#include "zlib.h"


namespace
{
const char* SIGNATURE = "VTA";
const uint32_t VERSION = 1;

}

namespace ImageIO
{

void EncodeRle7( std::vector<uint8_t>& dest, uint8_t bits, const void* src, size_t srcSize )
{
	if( srcSize == 0 )
	{
		return;
	}
	const uint8_t mask = ~( ( 1 << ( 8 - bits ) ) - 1 ) & ~0x1;
	auto pixels = static_cast<const uint8_t*>( src );
	auto current = pixels[0] & mask;
	uint8_t len = 0;
	for( size_t i = 1; i < srcSize; ++i )
	{
		auto next = pixels[i] & mask;
		if( next != current || len == 255 )
		{
			dest.push_back( current | ( len ? 1 : 0 ) );
			if( len )
			{
				dest.push_back( len );
			}
			len = 0;
			current = next;
		}
		else
		{
			++len;
		}
	}
	dest.push_back( current | ( len ? 1 : 0 ) );
	if( len )
	{
		dest.push_back( len );
	}
}

void EncodeRle7( std::vector<uint8_t>& dest, uint8_t bits, const void* src_, const void* base_, size_t srcSize )
{
	if( srcSize == 0 )
	{
		return;
	}
	auto src = static_cast<const uint8_t*>( src_ );
	auto base = static_cast<const uint8_t*>( base_ );

	const uint8_t mask = ~( ( 1 << ( 8 - bits ) ) - 1 ) & ~0x1;
	uint8_t current = ( src[0] & mask ) - ( base[0] & mask );
	uint8_t len = 0;
	for( size_t i = 1; i < srcSize; ++i )
	{
		uint8_t next = ( src[i] & mask ) - ( base[i] & mask );
		if( next != current || len == 255 )
		{
			dest.push_back( current | ( len ? 1 : 0 ) );

			if( len )
			{
				dest.push_back( len );
			}
			len = 0;
			current = next;
		}
		else
		{
			++len;
		}
	}
	dest.push_back( current | ( len ? 1 : 0 ) );
	if( len )
	{
		dest.push_back( len );
	}
}

void DecodeRle7( void* dest_, size_t size, const void* rle_ )
{
	auto rle = static_cast<const uint8_t*>( rle_ );
	auto dest = static_cast<uint8_t*>( dest_ );
	auto start = dest;
	for( size_t i = 0; size_t( dest - start ) < size; )
	{
		auto next = rle[i++];
		if( ( next & 1 ) != 0 )
		{
			size_t len = rle[i++];
			for( size_t j = 0; j < len + 1; ++j )
			{
				*dest++ = next & ~1;
			}
		}
		else
		{
			*dest++ = next & ~1;
		}
	}
}

void DecodeRle7( void* dest_, size_t size, const void* rle_, const void* base_ )
{
	auto rle = static_cast<const uint8_t*>( rle_ );
	auto base = static_cast<const uint8_t*>( base_ );
	uint8_t* dest = static_cast<uint8_t*>( dest_ );
	auto start = dest;
	for( size_t i = 0; size_t( dest - start ) < size; )
	{
		auto next = rle[i++];
		if( ( next & 1 ) != 0 )
		{
			size_t len = rle[i++];
			for( size_t j = 0; j < len + 1; ++j )
			{
				*dest++ = ( next & ~1 ) + *base++;
			}
		}
		else
		{
			*dest++ = ( next & ~1 ) + *base++;
		}
	}
}

namespace Vta
{

FileReader::FileReader() :
	m_stream( nullptr )
{
}

Result FileReader::SetStream( ICcpStream* stream )
{
	m_stream = stream;

	m_header = Header();
	m_grids.clear();
	m_offsets.clear();
	m_metadata.clear();

	if( !m_stream )
	{
		return Result::INVALID_DATA;
	}
	auto read = ReadHeader();
	if( !read )
	{
		m_header = Header();
		m_grids.clear();
		m_offsets.clear();
		m_metadata.clear();
		return read;
	}
	return Result::OK;
}

Result FileReader::ReadHeader()
{
	if( m_stream->Read( &m_header, sizeof( m_header ) ) != sizeof( m_header ) )
	{
		return Result::READ_FAILURE;
	}
	if( memcmp( m_header.signature, SIGNATURE, sizeof( m_header.version ) ) != 0 )
	{
		return Result::INVALID_HEADER;
	}
	if( m_header.version != VERSION )
	{
		return Result::HEADER_NOT_SUPPORTED;
	}
	m_grids.reserve( m_header.gridCount );
	for( uint32_t i = 0; i < m_header.gridCount; ++i )
	{
		GridInfo grid;
		if( m_stream->Read( &grid, sizeof( grid ) ) != sizeof( grid ) )
		{
			return Result::READ_FAILURE;
		}
		m_grids.push_back( grid );
	}
	m_offsets.resize( m_header.frameCount * m_header.gridCount );
	if( m_stream->Read( m_offsets.data(), m_offsets.size() * sizeof( uint64_t ) ) != m_offsets.size() * sizeof( uint64_t ) )
	{
		return Result::READ_FAILURE;
	}
	std::vector<char> str;
	for( uint32_t i = 0; i < m_header.metadataCount; ++i )
	{
		std::string key, value;
		uint32_t length;

		if( m_stream->Read( &length, sizeof( length ) ) != sizeof( length ) )
		{
			return Result::READ_FAILURE;
		}
		str.resize( length );
		if( m_stream->Read( str.data(), str.size() ) != str.size() )
		{
			return Result::READ_FAILURE;
		}
		key = std::string( begin( str ), end( str ) );

		if( m_stream->Read( &length, sizeof( length ) ) != sizeof( length ) )
		{
			return Result::READ_FAILURE;
		}
		str.resize( length );
		if( m_stream->Read( str.data(), str.size() ) != str.size() )
		{
			return Result::READ_FAILURE;
		}
		value = std::string( begin( str ), end( str ) );

		m_metadata.push_back( { key, value } );
	}
	return Result::OK;
}

bool FileReader::IsValid() const
{
	return m_header.version == VERSION;
}

uint32_t FileReader::GetVersion() const
{
	return m_header.version;
}

uint32_t FileReader::GetGridCount() const
{
	return m_header.gridCount;
}

uint32_t FileReader::GetFrameCount() const
{
	return m_header.frameCount;
}

GridInfo FileReader::GetGridInfo( uint32_t gridCount )
{
	return m_grids[gridCount];
}

const MetadataStrings& FileReader::GetMetadata() const
{
	return m_metadata;
}

std::pair<std::unique_ptr<uint8_t[]>, size_t> FileReader::GetFrame( uint32_t grid, uint32_t frame ) const
{
	if( grid >= m_header.gridCount || frame >= m_header.frameCount )
	{
		return {};
	}
	size_t idx = frame * m_header.gridCount + grid;
	m_stream->Seek( m_offsets[idx], ICcpStream::SO_BEGIN );
	uint64_t end = idx + 1 == m_offsets.size() ? m_header.dataEnd : m_offsets[idx + 1];
	size_t size = size_t( end - m_offsets[idx] );
	std::unique_ptr<uint8_t[]> data( new uint8_t[size] );
	if( m_stream->Read( data.get(), size ) != size )
	{
		return {};
	}
	return { std::move( data ), size };
}




FileWriter::FileWriter( const std::vector<GridInfo>& grids, uint32_t frames )
{
	memcpy( m_header.signature, SIGNATURE, 4 );
	m_header.version = VERSION;
	m_header.frameCount = frames;
	m_header.gridCount = uint32_t( grids.size() );
	m_grids = grids;
	m_prevFrames.resize( grids.size() );
	m_frames.resize( frames * grids.size() );
}

void FileWriter::AddFrame( uint32_t grid, uint32_t frame, const HostBitmap& bitmap )
{
	if( grid >= m_header.gridCount && frame >= m_header.frameCount )
	{
		return;
	}
	size_t srcSize;
	const void* srcData;

	if( m_grids[grid].encoding == Encoding::None )
	{
		srcSize = bitmap.GetRawDataSize();
		srcData = bitmap.GetRawData();
	}
	else
	{
		uint8_t bits;
		switch( m_grids[grid].encoding )
		{
		case Encoding::Rle7_5:
			bits = frame == 0 ? 7 : 5;
			break;
		case Encoding::Rle6:
			bits = 6;
			break;
		default:
			bits = 7;
			break;
		}
		m_prevFrames[grid].resize( bitmap.GetRawDataSize() );
		if( frame == 0 )
		{
			EncodeRle7( m_scratchData, bits, bitmap.GetRawData(), bitmap.GetRawDataSize() );
			DecodeRle7( m_prevFrames[grid].data(), m_prevFrames[grid].size(), m_scratchData.data() );
		}
		else
		{
			EncodeRle7( m_scratchData, bits, bitmap.GetRawData(), m_prevFrames[grid].data(), bitmap.GetRawDataSize() );
			DecodeRle7( m_prevFrames[grid].data(), m_prevFrames[grid].size(), m_scratchData.data(), m_prevFrames[grid].data() );
		}

		srcSize = m_scratchData.size();
		srcData = m_scratchData.data();
	}

	std::vector<uint8_t> compressed;
	unsigned long size = compressBound( static_cast<unsigned long>( srcSize ) );
	compressed.resize( size );
	compress( compressed.data(), &size, static_cast<const unsigned char*>( srcData ), static_cast<unsigned long>( srcSize ) );
	compressed.resize( size );

	m_frames[m_header.gridCount * frame + grid] = std::move( compressed );
	m_scratchData.clear();
}

void FileWriter::SetMetadata( const char* key, const char* value )
{
	for( auto& md : m_metadata )
	{
		if( md.first == key )
		{
			md.second = value;
			return;
		}
	}
	m_metadata.push_back( { key, value } );
}

void FileWriter::SetGridName( uint32_t grid, const char* name )
{
	memset( m_grids[grid].name, 0, sizeof( GridInfo::name ) );
	strncpy_s( m_grids[grid].name, name, sizeof( GridInfo::name ) );
	m_grids[grid].name[sizeof( GridInfo::name ) - 1] = 0;
}

Result FileWriter::WriteData( ICcpStream& stream ) const
{
	size_t start = sizeof( m_header ) + m_header.gridCount * sizeof( GridInfo ) + m_frames.size() * sizeof( uint64_t );
	for( auto& md : m_metadata )
	{
		start += sizeof( uint32_t ) + md.first.size();
		start += sizeof( uint32_t ) + md.second.size();
	}

	size_t total = 0;
	for( auto& data : m_frames )
	{
		total += data.size();
	}
	auto header = m_header;
	header.dataEnd = uint64_t( total + start );

#define WRITE_DATA( x, s )                      \
	if( stream.Write( ( x ), ( s ) ) != ( s ) ) \
	{                                           \
		return Result::WRITE_FAILURE;           \
	}
#define WRITE_VALUE( x ) WRITE_DATA( &x, sizeof( x ) )

	WRITE_VALUE( header );
	for( auto& grid : m_grids )
	{
		WRITE_VALUE( grid );
	}
	uint64_t offset = uint64_t( start );
	for( auto& frame : m_frames )
	{
		WRITE_VALUE( offset );
		offset += uint64_t( frame.size() );
	}
	for( auto& md : m_metadata )
	{
		uint32_t len;
		len = uint32_t( md.first.size() );
		WRITE_VALUE( len );
		WRITE_DATA( md.first.data(), len );
		len = uint32_t( md.second.size() );
		WRITE_VALUE( len );
		WRITE_DATA( md.second.data(), len );
	}
	for( auto& frame : m_frames )
	{
		WRITE_DATA( frame.data(), frame.size() );
	}
	return Result::OK;
}


FrameDecoder::FrameDecoder( FileReader& reader, uint32_t grid ) :
	m_reader( reader ),
	m_grid( grid ),
	m_frame( 0 )
{
	if( reader.IsValid() )
	{
		m_gridInfo = m_reader.GetGridInfo( grid );
		m_bitmap.CreateVolume( m_gridInfo.width, m_gridInfo.height, m_gridInfo.depth, 1, m_gridInfo.format );
		if( m_gridInfo.encoding != Encoding::None )
		{
			m_rleBuffer.resize( m_bitmap.GetRawDataSize() );
		}
		DecodeFrame();
	}
}

const HostBitmap& FrameDecoder::GetFrameBitmap() const
{
	return m_bitmap;
}

void FrameDecoder::AdvanceFrame()
{
	++m_frame;
	if( m_frame >= m_reader.GetFrameCount() )
	{
		m_frame = 0;
	}
	DecodeFrame();
}

void FrameDecoder::Restart()
{
	m_frame = 0;
	DecodeFrame();
}

void FrameDecoder::DecodeFrame()
{
	auto frame = m_reader.GetFrame( m_grid, m_frame );
	if( m_gridInfo.encoding == Encoding::None )
	{
		unsigned long size = static_cast<unsigned long>( m_bitmap.GetRawDataSize() );
		uncompress( reinterpret_cast<unsigned char*>( m_bitmap.GetRawData() ), &size, frame.first.get(), static_cast<unsigned long>( frame.second ) );
	}
	else
	{
		unsigned long size = static_cast<unsigned long>( m_rleBuffer.size() );
		uncompress( m_rleBuffer.data(), &size, frame.first.get(), static_cast<unsigned long>( frame.second ) );
		if( m_frame == 0 )
		{
			DecodeRle7( m_bitmap.GetRawData(), m_bitmap.GetRawDataSize(), m_rleBuffer.data() );
		}
		else
		{
			DecodeRle7( m_bitmap.GetRawData(), m_bitmap.GetRawDataSize(), m_rleBuffer.data(), m_bitmap.GetRawData() );
		}
	}
}

void RegisterHandler()
{
	static bool s_registered = false;
	if( !s_registered )
	{
		ImageFormatFunctions funcs = { &IsVtaExtension, &ReadImage, &IsSaveSupported, &Save };
		RegisterImageHandler( funcs );
		s_registered = true;
	}
}

bool IsVtaExtension( const wchar_t* ext )
{
	return ( ext[0] == L'v' || ext[0] == L'V' ) &&
		( ext[1] == L't' || ext[1] == L'T' ) &&
		( ext[2] == L'a' || ext[2] == L'A' ) &&
		ext[3] == 0;
}

Result ReadImage( ICcpStream& src, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata )
{
	FileReader reader;
	auto status = reader.SetStream( &src );
	if( !status )
	{
		return status;
	}
	if( reader.GetFrameCount() == 0 || reader.GetGridCount() == 0 )
	{
		return Result::INVALID_DATA;
	}
	FrameDecoder decoder( reader, 0 );
	if( !decoder.GetFrameBitmap().IsValid() )
	{
		return Result::INVALID_DATA;
	}
	if( !bitmap.CreateFromBitmapDimensions( decoder.GetFrameBitmap() ) )
	{
		return Result::OUT_OF_MEMORY;
	}
	memcpy( bitmap.GetRawData(), decoder.GetFrameBitmap().GetRawData(), bitmap.GetRawDataSize() );
	if( metadata )
	{
		metadata->metadata = reader.GetMetadata();
	}
	return Result::OK;
}

Result IsSaveSupported( const BitmapDimensions& bd )
{
	if( bd.GetType() == TEX_TYPE_3D && 
		bd.GetFormat() == PIXEL_FORMAT_R8_UNORM &&
		bd.GetArraySize() < 2 &&
		bd.GetMipCount() == 1 )
	{
		return Result::OK;
	}
	return Result::SAVE_NOT_SUPPORTED;
}

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

	GridInfo grid;
	grid.format = PIXEL_FORMAT_R8_UNORM;
	grid.width = image.GetWidth();
	grid.height = image.GetHeight();
	grid.depth = image.GetDepth();
	grid.encoding = Encoding::Rle7;
	std::vector<GridInfo> grids = { grid };

	FileWriter writer( grids, 1 );
	writer.AddFrame( 0, 0, image );
	if( metadata )
	{
		for( auto& md : metadata->metadata )
		{
			writer.SetMetadata( md.first.c_str(), md.second.c_str() );
		}
	}
	return writer.WriteData( output );
}

}
}