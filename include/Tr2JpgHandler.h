#pragma once
#ifndef Tr2JpgHandler_h
#define Tr2JpgHandler_h

#include "Tr2ImageHandler.h"

class Tr2JpgHandler : public Tr2ImageHandler
{
public:
	Tr2JpgHandler( const wchar_t* sourceName = 0 );
	~Tr2JpgHandler();

	bool ReadHeader( ICcpStream* src );
	Tr2RenderContextEnum::PixelFormat GetFormat() const ;
	bool ReadImage( ICcpStream* src );
	unsigned GetBlockByteSize() const;
	unsigned GetOffset( unsigned mipLevel, unsigned face ) const;

	virtual bool IsSaveSupported( const Tr2BitmapDimensions& bd );
	virtual bool Save( const ImageIO::HostBitmap& image, ICcpStream* output );
	
	struct InputData
	{
		uint8_t* start;
		unsigned dataSize;
		jmp_buf m_jmpBuf;
	};

private:
	struct Impl;
	std::unique_ptr<Impl>	m_impl;
	
	TrackableStdVector<unsigned char>	m_rgbData;
	TrackableStdVector<unsigned char>	m_compressedData;

	unsigned m_channels;

	InputData m_clientData;
};

#endif//Tr2JpgHandler_h