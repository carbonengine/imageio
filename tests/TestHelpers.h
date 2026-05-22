// Copyright © 2014 CCP ehf.

#pragma once
#ifndef TestHelpers_H
#define TestHelpers_H

#include "resourcesInclude/_all.h"

#define TEST_FILE( name, extension )															\
	CCP_CONCATENATE( L, #name ) L"." CCP_CONCATENATE( L, #extension ),							\
	CCP_CONCATENATE( CCP_CONCATENATE( CCP_CONCATENATE( s_, name ), _ ), extension ),			\
	sizeof( CCP_CONCATENATE( CCP_CONCATENATE( CCP_CONCATENATE( s_, name ), _ ), extension ) )

typedef void ( *GetPixelDataFunction )( ImageIO::HostBitmap&, const ImageIO::BitmapDimensions& dimensions );

template <typename PixelType, PixelType pixelValue>
void CheckTopLeftPixel( ImageIO::HostBitmap& bitmap, const ImageIO::BitmapDimensions& dimensions )
{
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( bitmap.GetRawData() )[0] );
}

template <typename PixelType, PixelType pixelValue>
void CheckBottomRightPixel( ImageIO::HostBitmap& bitmap, const ImageIO::BitmapDimensions& dimensions )
{
	size_t offset = ( dimensions.GetWidth() * dimensions.GetHeight() * dimensions.GetDepth() ) * GetBytesPerPixel( dimensions.GetFormat() ) - sizeof( PixelType );
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( bitmap.GetRawData() + offset )[0] );
}

template <ImageIO::CubemapFace face, typename PixelType, PixelType pixelValue>
void CheckFaceTopLeftPixel( ImageIO::HostBitmap& bitmap, const ImageIO::BitmapDimensions& dimensions )
{
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( bitmap.GetMipRawData( 0, face ) )[0] );
}

template <ImageIO::CubemapFace face, typename PixelType, PixelType pixelValue>
void CheckFaceBottomRightPixel( ImageIO::HostBitmap& bitmap, const ImageIO::BitmapDimensions& dimensions )
{
	size_t offset = ( dimensions.GetWidth() * dimensions.GetHeight() ) * GetBytesPerPixel( dimensions.GetFormat() ) - sizeof( PixelType );
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( bitmap.GetMipRawData( 0, face ) + offset )[0] );
}

template <uint32_t mip, typename PixelType, PixelType pixelValue>
void CheckMipTopLeftPixel( ImageIO::HostBitmap& bitmap, const ImageIO::BitmapDimensions& dimensions )
{
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( bitmap.GetMipRawData( mip ) )[0] );
}

template <uint32_t mip, typename PixelType, PixelType pixelValue>
void CheckMipBottomRightPixel( ImageIO::HostBitmap& bitmap, const ImageIO::BitmapDimensions& dimensions )
{
	size_t offset = ( bitmap.GetMipWidth( mip ) * dimensions.GetMipHeight( mip ) * dimensions.GetMipDepth( mip ) ) * GetBytesPerPixel( dimensions.GetFormat() ) - sizeof( PixelType );
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( bitmap.GetMipRawData( mip ) + offset )[0] );
}

struct TestImage
{
	const wchar_t* fileNameWide;
	uint8_t* data;
	size_t dataSize;
	ImageIO::BitmapDimensions dimensions;
	GetPixelDataFunction imageChecks[16];
};

void TestReadImage( const TestImage& image );
void AssertReadImageFails( const TestImage& image );
void AssertReadTruncatedImageFails( const TestImage& image, size_t truncatedSize );

void AssertCanSaveImage( const TestImage& image );
void AssertCanSaveImageContentsMayDiffer( const TestImage& image );

#endif