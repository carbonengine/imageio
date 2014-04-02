////////////////////////////////////////////////////////////
//
//    Creator:   Filipp Pavlov
//    Created:   May 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2TgaHandler_h
#define Tr2TgaHandler_h

#include "Tr2ImageHandler.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2TgaHandler is an image handler that deals with TGA images. 
// See Also:
//   Tr2ImageHandler
// --------------------------------------------------------------------------------------
class Tr2TgaHandler : public Tr2ImageHandler
{
public:
	Tr2TgaHandler( const wchar_t* sourceName = 0 );

	bool ReadHeader( ICcpStream* src );
	Tr2RenderContextEnum::PixelFormat GetFormat() const ;
	bool ReadImage( ICcpStream* src );
	unsigned GetBlockByteSize() const;
	unsigned GetOffset( unsigned mipLevel, unsigned face ) const;

	bool IsSaveSupported( const Tr2BitmapDimensions& bd );
	bool Save( const ImageIO::HostBitmap& image, ICcpStream* output );
	bool GenerateMips();

	static bool IsSaveSupported( Tr2RenderContextEnum::PixelFormat format );
	static bool SaveHeader( uint32_t width, uint32_t height, Tr2RenderContextEnum::PixelFormat format, ICcpStream* output );
	static bool SaveRows( uint32_t width, uint32_t height, Tr2RenderContextEnum::PixelFormat format, const void* data, ICcpStream* output );

private:
	// ----------------------------------------------------------------------------------
	// Description:
	//   TGA file header structure 
	// ----------------------------------------------------------------------------------
	#pragma pack(push)
	#pragma pack(1)
	struct Header
	{
		// Length of additional header data
		uint8_t idLength;
		// If file containts paletted colors
		uint8_t colorMapType;
		// Image type (pixel format and compression)
		uint8_t imageType;
		// Pallete offset
		uint16_t colorMapStart;
		// Palette length
		uint16_t colorMapLength;
		// Palette bits per pixel
		uint8_t colorMapBpp;
		// Image rectangle
		uint16_t left;
		uint16_t top;
		uint16_t right;
		uint16_t bottom;
		// Bits per pixel for image data
		uint8_t bpp;
		uint8_t imageDescriptor;
	};
	#pragma pack(pop)

	bool ReadPalette( ICcpStream* stream, unsigned char*& palette );
	bool ReadRawData( ICcpStream* stream, unsigned char* palette );
	bool ReadRLEData( ICcpStream* stream, unsigned char* palette );	
	bool GenerateMipLevel( uint8_t* source, unsigned width, unsigned height, uint8_t* destination );

	// Read TGA header
	Header m_header;
	// Entire file size
	size_t m_fileSize;
	// Image data size
	size_t m_imageSize;
};

#endif//Tr2TgaHandler_h_h