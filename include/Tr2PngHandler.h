#pragma once

#ifndef Tr2PngHandler_h
#define Tr2PngHandler_h

#include "Tr2ImageHandler.h"
#include "png.h"

class Tr2PngHandler : public Tr2ImageHandler
{
public:
	Tr2PngHandler( const wchar_t* sourceName );
	virtual ~Tr2PngHandler();

	virtual bool ReadHeader( ICcpStream* src );
	virtual bool ReadImage( ICcpStream* src );

	virtual Tr2RenderContextEnum::PixelFormat GetFormat() const;

	virtual unsigned GetBlockByteSize() const;

	bool IsValid() const;

	virtual unsigned GetOffset( unsigned mipLevel, unsigned face ) const;

	virtual bool IsSaveSupported( const Tr2BitmapDimensions& bd );
	virtual bool Save( const ImageIO::HostBitmap& image, ICcpStream* output );
    
private:
	bool IsSupported() const;
	bool DoReadHeader();
	bool DoReadImage( png_bytep* rowPtrs );
	bool DoSaveImage( png_structp png, png_infop info );

	unsigned GetColorType() const	{ return IsValid() ? png_get_color_type  ( m_png, m_info ) : 0; }

	png_structp	m_png;
	png_infop	m_info;
	
	unsigned	m_channels;

	jmp_buf		m_jmpBuf;
	bool		m_haveError;

	void ReadData   ( ICcpStream* stream, png_bytep data, png_size_t length );
	void UserError  ( png_const_charp errorMsg);
    void UserWarning( png_const_charp warningMsg);

	static void PNGAPI ReadData_thunk( png_structp pngPtr, png_bytep data, png_size_t length );
	static void PNGAPI UserError_thunk  ( png_structp pngPtr, png_const_charp errorMsg);
    static void PNGAPI UserWarning_thunk( png_structp pngPtr, png_const_charp warningMsg);

	static png_voidp MemoryAlloc( png_structp, png_alloc_size_t );
	static void MemoryFree( png_structp, png_voidp );

	Tr2PngHandler( const Tr2PngHandler & );
	Tr2PngHandler& operator=( const Tr2PngHandler &);
};

#endif // Tr2PngHandler_h
