#pragma once

#ifndef Tr2ImageHandler_h
#define Tr2ImageHandler_h

namespace ImageIO
{
class  HostBitmap;
}

// Tr2ImageHandler takes care of reading image data from a file and provides
// access to the data. This is a base class with several pure virtual functions,
// concrete classes such Tr2PngHandler, Tr2SddHandler and Tr2DdsHandler provide
// full implementations for different file types.
class Tr2ImageHandler
{
public:
	Tr2ImageHandler( const wchar_t* sourceName = 0 );
	virtual ~Tr2ImageHandler();

	void SetMipLevelSkipCount( unsigned int skip );
	void SetMipLevelMaxCount( unsigned int max );

	const wchar_t* GetSourceName() const;

	// Read header from source stream. Returns true if a valid header was read.
	virtual bool ReadHeader( ICcpStream* src ) = 0;

	// Returns the format of the texture being loaded.
	// Assumes that header has been successfully read, and determined to be 
	// valid - only call after ReadHeader returns true.
	virtual Tr2RenderContextEnum::PixelFormat GetFormat() const = 0;

	// Set a format, before actually loading, of what we want the desired texture to be.
	// This will _not_ trigger a very generic conversion routine that can convert from
	// anything to anything; instead a limited number of cases are supported, and the image
	// handler will convert when uploading to the GPU between what we want, and what the
	// PNG is giving us.
	virtual void SetDesiredFormat( Tr2RenderContextEnum::PixelFormat format );

	// Reads image from source stream. 
	// Assumes that header has been successfully read, and determined to be 
	// valid - only call after ReadHeader returns true.
	virtual bool ReadImage( ICcpStream* src ) = 0;

	// Assume that non-dds formats aren't going to support volumes and cubemaps, so return defaults of false here:
	virtual bool IsCubeTexture() const { return false; }
	virtual bool IsVolumeTexture() const { return false; }

	// return the dds block size, or 0 if there's no compression.
	virtual unsigned GetBlockByteSize() const = 0;

	// Allow runtime generation of mipmaps.  Automatically called by TriTextureRes, 
	// but really only intended for tga's when running from content.
	virtual bool GenerateMips() { return true; }

	bool IsCompressed() const { return GetBlockByteSize() != 0; }
	
	// Get the size of a particular mip level, based on the data in the header
	uint32_t GetMipLevelSize( uint32_t mipLevel ) const;
	uint32_t GetMipLevelHeight( uint32_t mipLevel ) const;
	uint32_t GetMipLevelWidth( uint32_t mipLevel ) const;
	uint32_t GetMipLevelDepth( uint32_t mipLevel ) const;

	uint32_t GetMipLevelCount() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDepth() const;	// depth for volume textures, not the bit depth!
	uint32_t GetFaceSize() const;

	// return the offset where this face and miplevel would be, relative to the start of m_data.
	// This does not include the size of the header, ie. we're dealing strictly with texels.
	virtual unsigned GetOffset( unsigned mipLevel, unsigned face ) const = 0;

	virtual bool Save( const ImageIO::HostBitmap&, ICcpStream* ) { return false; }

	virtual bool IsSaveSupported( const Tr2BitmapDimensions& ) { return false; }
	
	unsigned int GetTotalDataSize() const;

	// any cut out rectangles; specified in 0...1 range
	float	m_cutoutX;
	float	m_cutoutY;
	float	m_cutoutWidth;
	float	m_cutoutHeight;

	static void AddMargin(	Tr2RenderContextEnum::PixelFormat format,
							const unsigned char* source,
							unsigned sourceWidth, unsigned sourceHeight,
							unsigned marginToAdd,
							std::vector<unsigned char> &output, 
							unsigned &outputPitch );

	virtual	unsigned char* GetMipBytes( unsigned mipLevel, unsigned face = 0 );

	unsigned GetPitch( unsigned mipLevel );

protected:
	
	std::wstring m_sourceName;

	unsigned m_width;
	unsigned m_height;
	unsigned m_bitsPerPixel;
	unsigned m_mipLevelCount;
	unsigned m_volumeDepth;

	unsigned int m_dataSize;	
	uint8_t* m_data;

	unsigned int m_mipLevelSkipCount;
	unsigned int m_mipLevelMaxCount;

	// virtual so child class can do any pre- or post-tweaks to bitfields etc.
	virtual bool Convert24BitTo32Bit();

	// compute the number of mip levels to skip, and the count of how many to read, based on
	// m_mipLevelSkipCount and m_mipLevelMaxCount.  set mipCount to the actual number of mips initially.
	void	GetMipLevelRange( unsigned & skipCount, unsigned & mipCount );

	Tr2RenderContextEnum::PixelFormat m_desiredFormat;
	bool ConvertDesiredFormat();
	
private:
	Tr2ImageHandler( const Tr2ImageHandler& );
	Tr2ImageHandler& operator=( const Tr2ImageHandler& );
};

// factory helper to distinguish supported file formats.
Tr2ImageHandler* CreateImageHandler( const std::wstring &filePath );

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
	((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
	((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */


#endif
