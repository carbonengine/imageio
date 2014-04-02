#pragma once
#ifndef PsdHandler_H
#define PsdHandler_H

#include "Tr2ImageHandler.h"

namespace ImageIO
{

// --------------------------------------------------------------------------------------
// Description:
//   Tr2BmpHandler is an image handler that deals with BMP images. 
// See Also:
//   Tr2ImageHandler
// --------------------------------------------------------------------------------------
class PsdHandler : public Tr2ImageHandler
{
public:
	PsdHandler( const wchar_t* sourceName = 0 );

	bool ReadHeader( ICcpStream* src );
	Tr2RenderContextEnum::PixelFormat GetFormat() const ;
	bool ReadImage( ICcpStream* src );
	unsigned GetBlockByteSize() const;
	unsigned GetOffset( unsigned mipLevel, unsigned face ) const;

	virtual bool IsSaveSupported( const Tr2BitmapDimensions& bd );
	virtual bool Save( const ImageIO::HostBitmap& image, ICcpStream* output );
private:
	// ----------------------------------------------------------------------------------
	// Description:
	//   PSD file header structure 
	// ----------------------------------------------------------------------------------
	#pragma pack(push)
	#pragma pack(1)
	struct Header
	{
        uint32_t signature;
        uint16_t version;
        uint8_t reserved[6];
        uint16_t channelCount;
        uint32_t height;
        uint32_t width;
        uint16_t depth;
        uint16_t colorMode;
	};
	#pragma pack(pop)

	bool ReadRleData( ICcpStream* src );
	bool ReadUncompressedData( ICcpStream* src );

	Header m_header;
	uint16_t m_compression;
};

}

#endif