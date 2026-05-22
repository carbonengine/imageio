// Copyright © 2014 CCP ehf.

#pragma once

#ifndef Tr2ImageHandler_h
#define Tr2ImageHandler_h

#include "HostBitmap.h"
#include "ImageIOResult.h"

namespace ImageIO
{

// --------------------------------------------------------------------------------------
// Description:
//   Information in image cutout from PNGs.
// --------------------------------------------------------------------------------------
struct Cutout
{
	Cutout()
		:x( 0.f ),
		y( 0.f ),
		width( 1.f ),
		height( 1.f )
	{
	}

	float x;
	float y;
	float width;
	float height;
};

typedef std::vector<std::pair<std::string, std::string>> MetadataStrings;

// --------------------------------------------------------------------------------------
// Description:
//   Image metadata loaded from image file.
// --------------------------------------------------------------------------------------
struct Metadata
{
	Cutout cutout;
	MetadataStrings metadata;
};

// --------------------------------------------------------------------------------------
// Description:
//   Parameters passed to ReadImage function.
// --------------------------------------------------------------------------------------
struct LoadParameters
{
	LoadParameters( const wchar_t* filename, uint32_t mipLevelSkipCount = 0, uint32_t mipLevelMaxCount = std::numeric_limits<uint32_t>::max() );
	void GetMipLevelRange( uint32_t width, uint32_t height, uint32_t& skipCount, uint32_t& mipCount ) const;

	// Filename used for determining image type and for logging
	std::wstring m_filename;
	// How many mip levels to skip when loading images with mip levels
	uint32_t m_mipLevelSkipCount;
	// Maximum number of mips to load
	uint32_t m_mipLevelMaxCount;
};

typedef bool ( *CheckExtensionFunc )( const wchar_t* extension );
typedef Result ( *ReadImageFunc )( ICcpStream& src, const LoadParameters& loadParameters, HostBitmap& bitmap, Metadata* metadata );
typedef Result ( *IsSaveSupportedFunc )( const BitmapDimensions& bd );
typedef Result ( *SaveFunc )( const HostBitmap& image, ICcpStream& output, const Metadata* metadata );

// --------------------------------------------------------------------------------------
// Description:
//   Collection of function pointers that describe image format handling (poor man 
//   interface).
// --------------------------------------------------------------------------------------
struct ImageFormatFunctions
{
	CheckExtensionFunc checkExtension;
	ReadImageFunc readImage;
	IsSaveSupportedFunc isSaveSupported;
	SaveFunc save;
};

void RegisterImageHandler( const ImageFormatFunctions& imageHandler );
Result ReadImage( ICcpStream& src, const LoadParameters& loadParameters, HostBitmap& bitmap, Metadata* metadata = nullptr );
Result IsSaveSupported( const wchar_t* sourceName, const BitmapDimensions& dimensions );
Result SaveImage( const wchar_t* sourceName, const HostBitmap& bitmap, ICcpStream& dest, const Metadata* metadata = nullptr );

}


#endif
