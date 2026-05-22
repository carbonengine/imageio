// Copyright © 2023 CCP ehf.

#pragma once

#include "nanovdb/NanoVDB.h"
#include "VtaHandler.h"

namespace ImageIO
{

class HostBitmap;

enum class NanoVDBStatus
{
	OK,
	IO_ERROR,
	INVALID_GRID,
	INVALID_FORMAT,
};


struct NanoVDBGridMetadata
{
	std::string name;
	int32_t width, height, depth;
	nanovdb::GridType type;
};

bool RasterizeNanoVDB( const char* vdbPath, uint32_t gridNumber, PixelFormat format, int32_t bpp, HostBitmap& bitmap );
bool GetNanoVDBMetaData( const char* vdbPath, std::vector<NanoVDBGridMetadata>& metaData );

struct RasterizeGridInfo
{
	uint32_t index;
	PixelFormat format;
	Vta::Encoding encoding;
};
std::vector<uint8_t> NanoVDBToVTA( const std::vector<std::string>& vdbPaths, const std::vector<RasterizeGridInfo>& grids );

std::vector<uint8_t> DownsampleVTA( const void* source, size_t sourceSize );

}