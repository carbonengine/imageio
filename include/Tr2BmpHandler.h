////////////////////////////////////////////////////////////
//
//    Creator:   Filipp Pavlov
//    Created:   July 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2BmpHandler_h
#define Tr2BmpHandler_h

#include "Tr2ImageHandler.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2BmpHandler is an image handler that deals with BMP images. 
// See Also:
//   Tr2ImageHandler
// --------------------------------------------------------------------------------------
class Tr2BmpHandler : public Tr2ImageHandler
{
public:
	Tr2BmpHandler( const wchar_t* sourceName = 0 );

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
	//   BMP file header structure 
	// ----------------------------------------------------------------------------------
	#pragma pack(push)
	#pragma pack(1)
	struct BmpHeader
	{
		// signature - 'BM'
		unsigned short type;     
		// file size in bytes
		unsigned size;   
		// 0
		unsigned short reserved1;     
		// 0
		unsigned short reserved2; 
		// offset to bitmap
		unsigned offBits;  
	};
	// ----------------------------------------------------------------------------------
	// Description:
	//   Minimal (OS/2) DIB header structure 
	// ----------------------------------------------------------------------------------
	struct MinDibHeader
	{
		// size of this struct (40)
		unsigned structSize;
		// bmap width in pixels
		unsigned width; 
		// bmap height in pixels
		unsigned height;    
		// num planes - always 1
		unsigned short planes;  
		// bits per pixel
		unsigned short bitCount;
	};
	// ----------------------------------------------------------------------------------
	// Description:
	//   Windows DIB header structure 
	// ----------------------------------------------------------------------------------
	struct DibHeader: public MinDibHeader
	{
		// compression flag
		unsigned compression;   
		// image size in bytes
		unsigned sizeImage;     
		// horz resolution
		int xPelsPerMeter; 
		// vert resolution
		int yPelsPerMeter; 
		// 0 -> color table size
		unsigned clrUsed;
		// important color count
		unsigned clrImportant;  
	};
	#pragma pack(pop)

	// BMP header
	BmpHeader m_bmpHeader;
	// DIB header
	DibHeader m_dibHeader;
	// Entire file size
	size_t m_fileSize;
	// Image data size
	size_t m_imageSize;
};

#endif//Tr2BmpHandler_h_h