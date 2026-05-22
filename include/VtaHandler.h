// Copyright © 2023 CCP ehf.

#pragma once
#include "Tr2ImageHandler.h"

namespace ImageIO
{

void EncodeRle7( std::vector<uint8_t>& dest, uint8_t bits, const void* src, size_t srcSize );
void EncodeRle7( std::vector<uint8_t>& dest, uint8_t bits, const void* src, const void* base, size_t srcSize );
void DecodeRle7( void* dest, size_t destSize, const void* rle );
void DecodeRle7( void* dest, size_t destSize, const void* rle, const void* base );

namespace Vta
{

enum class Encoding : uint32_t
{
	None,
	Rle7,
	Rle7_5,
	Rle6,
};

struct GridInfo
{
	PixelFormat format;
	Encoding encoding;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	char name[32];
};

struct Header
{
	uint8_t signature[4] = {};
	uint32_t version = 0;

	uint32_t gridCount = 0;
	uint32_t frameCount = 0;
	uint32_t metadataCount = 0;
	uint32_t padding = 0;
	uint64_t dataEnd = 0;

	// Header is followed by:
	// GridInfo grids[]
	// uint64_t offsets[frameCount * gridCount];
	// metadata strings
	// image data
};



class FileReader
{
public:
	FileReader();

	Result SetStream( ICcpStream* stream );

	bool IsValid() const;

	uint32_t GetVersion() const;
	uint32_t GetGridCount() const;
	uint32_t GetFrameCount() const;

	GridInfo GetGridInfo( uint32_t gridCount );
	const MetadataStrings& GetMetadata() const;

	std::pair<std::unique_ptr<uint8_t[]>, size_t> GetFrame( uint32_t grid, uint32_t frame ) const;

private:
	Result ReadHeader();

	ICcpStream* m_stream;
	Header m_header;
	std::vector<GridInfo> m_grids;
	std::vector<uint64_t> m_offsets;
	MetadataStrings m_metadata;
};


class FileWriter
{
public:
	FileWriter( const std::vector<GridInfo>& grids, uint32_t frames );

	void AddFrame( uint32_t grid, uint32_t frame, const HostBitmap& bitmap );
	void SetMetadata( const char* key, const char* value );
	Result WriteData( ICcpStream& stream ) const;
	void SetGridName( uint32_t grid, const char* name );

private:
	Header m_header;
	std::vector<GridInfo> m_grids;
	std::vector<std::vector<uint8_t>> m_frames;
	std::vector<std::vector<uint8_t>> m_prevFrames;
	MetadataStrings m_metadata;
	std::vector<uint8_t> m_scratchData;
};


class FrameDecoder
{
public:
	FrameDecoder( FileReader& reader, uint32_t grid );

	const HostBitmap& GetFrameBitmap() const;
	void AdvanceFrame();
	void Restart();

private:
	void DecodeFrame();

	FileReader& m_reader;
	uint32_t m_grid;
	GridInfo m_gridInfo;
	HostBitmap m_bitmap;
	uint32_t m_frame;
	std::vector<uint8_t> m_rleBuffer;
};


void RegisterHandler();
bool IsVtaExtension( const wchar_t* extension );
Result ReadImage( ICcpStream& src, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata );
Result IsSaveSupported( const BitmapDimensions& bd );
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata );

}


}
