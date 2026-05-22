// Copyright © 2014 CCP ehf.

#pragma once
#ifndef HostBitmap_H
#define HostBitmap_H

#include "BitmapDimensions.h"

class Tr2ImageHandler;

namespace ImageIO
{

class HostBitmap: public BitmapDimensions
{
public:
    HostBitmap();
	HostBitmap( HostBitmap&& other );
	virtual ~HostBitmap();

	HostBitmap& operator=( HostBitmap&& other );

	bool IsValid() const;
	virtual void Destroy();

	bool Create( unsigned width, unsigned height, unsigned mipCount, PixelFormat format );
	bool Create2DArray( unsigned width, unsigned height, unsigned mipCount, unsigned arraySize, PixelFormat format );
	bool CreateCube( unsigned width, unsigned mipCount, PixelFormat format );
	bool CreateVolume( unsigned width, unsigned height, unsigned depth, unsigned mipCount, PixelFormat format );
	bool CreateFromBitmapDimensions( const BitmapDimensions& dimensions );

	unsigned GetPitch() const;

	const char* GetRawData() const;
	char* GetRawData();
	const char* GetRawData( unsigned x, unsigned y ) const;
	char* GetRawData( unsigned x, unsigned y );
	size_t GetRawDataSize() const;
	size_t GetArrayElementSize() const;

	const char* GetMipRawData( unsigned level, uint32_t arrayIndex = 0 ) const;
	char* GetMipRawData( unsigned level, uint32_t arrayIndex = 0 );

	bool PopulateMargin( unsigned margin );

	bool CopyChannel( HostBitmap *source, unsigned srcChannel, unsigned dstChannel );

	bool Downsample2x2();
	bool Crop( unsigned left, unsigned top, unsigned right, unsigned bottom );

	bool RotateFaceClockwise( unsigned face, unsigned times );
	bool ConvertCrossmapToCubemap();
	bool ConvertToVolume();

	bool ChangeFormat( PixelFormat format );

	bool GenerateMipMaps( unsigned levels = 0 );
	bool DropMipMaps();
	bool ConvertFormat( PixelFormat format );
	bool GetAverageColor( float &r, float &g, float &b, float &a );
	bool GetPixel( uint32_t x, uint32_t y, float &r, float &g, float &b, float &a ) const;

protected:
	bool CheckForMatch( const BitmapDimensions& bd, bool checkDimensions, bool& alphaConvert, const char* log );

	std::string m_name;

	CcpMallocBuffer m_data;
private:
	bool GenerateMipLevel( uint8_t* source, unsigned width, unsigned height, uint8_t* destination );
};

}

#endif