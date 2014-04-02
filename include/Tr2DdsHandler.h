#pragma once

#ifndef Tr2DdsHandler_h_
#define Tr2DdsHandler_h_

#include "Tr2ImageHandler.h"

//
// Tr2DdsHandler loads, creates, and saves textures. The source data is assumed to
// be in DDS format. The loader separates the loading and creation calls to
// support background loading, while creation calls can take place on the main
// thread.
//
// The loader can do some basic format conversions, but mostly creates textures
// exactly as described in the DDS file.
//
class Tr2DdsHandler : public Tr2ImageHandler
{
public:
	Tr2DdsHandler( const wchar_t* sourceName = 0 );
	~Tr2DdsHandler();

	// Read header from source stream. Returns true if a valid header was read.
	bool ReadHeader( ICcpStream* src );

	bool IsSaveSupported( const Tr2BitmapDimensions& bd );

	// Construct DDS header from the texture's format and parameters
	bool BuildHeader( const Tr2BitmapDimensions& bd );

	// Returns the format of the texture being loaded.
	// Assumes that header has been successfully read, and determined to be 
	// valid - only call after ReadHeader returns true.
	virtual Tr2RenderContextEnum::PixelFormat GetFormat() const;

	// Reads data from source stream. 
	// Assumes that header has been successfully read, and determined to be 
	// valid - only call after ReadHeader returns true.
	bool ReadImage( ICcpStream* src );

	

	// Call this before loading first texture to determine texture formats supported.
	static void CacheSupportedFormats();

	unsigned GetBlockByteSize() const;

	bool IsCubeTexture() const;
	bool IsVolumeTexture() const;

	virtual bool Save( const ImageIO::HostBitmap& image, ICcpStream* output );

	struct DDS_PIXELFORMAT
	{
		uint32_t dwSize;			// 4
		uint32_t dwFlags;
		uint32_t dwFourCC;
		uint32_t dwRGBBitCount;	// 16
		uint32_t dwRBitMask;
		uint32_t dwGBitMask;
		uint32_t dwBBitMask;
		uint32_t dwABitMask;		// 32
	};

	// Helper to build a DDS pixelformat matching a given TriTextureRes
	static bool MakePixelFormat( DDS_PIXELFORMAT & ddspf, const Tr2BitmapDimensions& bd );

	unsigned GetOffset( unsigned mipLevel, unsigned face ) const;

protected:
	virtual bool Convert24BitTo32Bit();

private:

	struct DDS_HEADER
	{
		uint32_t dwFourCC;				// 4
		uint32_t dwSize;
		uint32_t dwHeaderFlags;
		uint32_t dwHeight;				// 16
		uint32_t dwWidth;
		uint32_t dwPitchOrLinearSize;
		uint32_t dwDepth;				// only if DDS_HEADER_FLAGS_VOLUME is set in dwHeaderFlags
		uint32_t dwMipMapCount;		// 32
		uint32_t dwReserved1[11];		// 76
		DDS_PIXELFORMAT ddspf;		// 108
		uint32_t dwSurfaceFlags;
		uint32_t dwCubemapFlags;		// 116
		uint32_t dwReserved2[3];		// 128
	};

	bool IsSupported() const;

	DDS_HEADER m_header;
	Tr2RenderContextEnum::PixelFormat m_format;

	void CopyHeaderValuesToMembers();	// from m_header to the base m_width etc.. members

	static std::pair<int32_t, Tr2RenderContextEnum::PixelFormat> FindDdsFormat( const DDS_PIXELFORMAT& pf );

	// cache the size of a full mip pyramid for a single face
	unsigned m_fullMipSize;
};

#endif
