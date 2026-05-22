// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "CcpMetadata.h"

namespace
{
	struct CcpMetadataHeader
	{
		char signature[8];
		uint32_t version;
		uint32_t count;
		uint32_t size;
	};

	struct CcpMetadataEntry
	{
		uint32_t keyOffset;
		uint32_t keyLength;
		uint32_t valueOffset;
		uint32_t valueLength;
	};

	static const char* CCP_METADATA_SIGNATURE = "CCP-META";
	static const uint32_t CCP_METADATA_VERSION = 1;
}

namespace ImageIO
{
	Result LoadCcpMetadata( ICcpStream& stream, Metadata& metadata )
	{
		metadata.metadata.clear();

		CcpMetadataHeader header;
		if( stream.Read( &header, sizeof( header ) ) != sizeof( header ) )
		{
			return Result::READ_FAILURE;
		}

		if( memcmp( header.signature, CCP_METADATA_SIGNATURE, sizeof( header.signature ) ) != 0 )
		{
			return Result::INVALID_HEADER;
		}
		if( header.version != CCP_METADATA_VERSION )
		{
			return Result::INVALID_HEADER;
		}
		if( header.count == 0 )
		{
			return Result::OK;
		}

		std::unique_ptr<uint8_t[]> data( new uint8_t[header.size] );
		if( !data )
		{
			return Result::OUT_OF_MEMORY;
		}
		if( stream.Read( data.get(), header.size ) != header.size )
		{
			return Result::READ_FAILURE;
		}

		metadata.metadata.reserve( header.count );
		const CcpMetadataEntry* entry = reinterpret_cast<const CcpMetadataEntry*>( data.get() );
		for( uint32_t j = 0; j < header.count; ++j )
		{
			metadata.metadata.push_back( std::make_pair(
				std::string( reinterpret_cast<const char*>( data.get() + entry->keyOffset ), entry->keyLength - 1 ),
				std::string( reinterpret_cast<const char*>( data.get() + entry->valueOffset ), entry->valueLength - 1 ) ) );
			++entry;
		}
		return Result::OK;
	}

	Result SaveCcpMetadata( ICcpStream& stream, const Metadata& metadata )
	{
		std::vector<CcpMetadataEntry> entries;
		entries.reserve( metadata.metadata.size() );

		uint32_t offset = uint32_t( sizeof( CcpMetadataEntry ) * metadata.metadata.size() );
		size_t size = 0;
		for( size_t i = 0; i < metadata.metadata.size(); ++i )
		{
			CcpMetadataEntry entry;
			entry.keyOffset = uint32_t( offset + size );
			entry.keyLength = uint32_t( metadata.metadata[i].first.length() + 1 );
			size += entry.keyLength;
			entry.valueOffset = uint32_t( offset + size );
			entry.valueLength = uint32_t( metadata.metadata[i].second.length() + 1 );
			size += entry.valueLength;
			entries.push_back( entry );
		}

		std::unique_ptr<char[]> strings( new char[size] );
		char* dest = strings.get();
		for( size_t i = 0; i < metadata.metadata.size(); ++i )
		{
			strcpy_s( dest, size, metadata.metadata[i].first.c_str() );
			dest += entries[i].keyLength;
			strcpy_s( dest, size, metadata.metadata[i].second.c_str() );
			dest += entries[i].valueLength;
		}

		CcpMetadataHeader metadataHeader;
		memcpy( metadataHeader.signature, CCP_METADATA_SIGNATURE, sizeof( metadataHeader.signature ) );
		metadataHeader.version = CCP_METADATA_VERSION;
		metadataHeader.count = uint32_t( metadata.metadata.size() );
		metadataHeader.size = uint32_t( offset + size );

		if( stream.Write( &metadataHeader, sizeof( metadataHeader ) ) != sizeof( metadataHeader ) )
		{
			return ImageIO::Result::WRITE_FAILURE;
		}
		if( stream.Write( entries.data(), entries.size() * sizeof( CcpMetadataEntry ) ) != entries.size() * sizeof( CcpMetadataEntry ) )
		{
			return ImageIO::Result::WRITE_FAILURE;
		}
		if( stream.Write( strings.get(), size ) != size )
		{
			return ImageIO::Result::WRITE_FAILURE;
		}
		return ImageIO::Result::OK;
	}
}