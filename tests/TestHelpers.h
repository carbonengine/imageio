#pragma once
#ifndef TestHelpers_H
#define TestHelpers_H

#include "resourcesInclude/_all.h"

#define TEST_FILE( name, extension )															\
	CCP_CONCATENATE( L, #name ) L"." CCP_CONCATENATE( L, #extension ),							\
	CCP_CONCATENATE( CCP_CONCATENATE( CCP_CONCATENATE( s_, name ), _ ), extension ),			\
	sizeof( CCP_CONCATENATE( CCP_CONCATENATE( CCP_CONCATENATE( s_, name ), _ ), extension ) )

typedef void (*GetPixelDataFunction)( Tr2ImageHandler&, const Tr2BitmapDimensions& dimensions );

template <typename PixelType, PixelType pixelValue>
void CheckTopLeftPixel( Tr2ImageHandler& handler, const Tr2BitmapDimensions& dimensions )
{
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( handler.GetMipBytes( 0 ) )[0] );
}

template <typename PixelType, PixelType pixelValue>
void CheckBottomRightPixel( Tr2ImageHandler& handler, const Tr2BitmapDimensions& dimensions )
{
	size_t offset = ( dimensions.GetWidth() * dimensions.GetHeight() * dimensions.GetDepth() ) * GetBytesPerPixel( dimensions.GetFormat() ) - sizeof( PixelType );
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( handler.GetMipBytes( 0 ) + offset )[0] );
}

template <Tr2RenderContextEnum::CubemapFace face, typename PixelType, PixelType pixelValue>
void CheckFaceTopLeftPixel( Tr2ImageHandler& handler, const Tr2BitmapDimensions& dimensions )
{
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( handler.GetMipBytes( 0, face ) )[0] );
}

template <Tr2RenderContextEnum::CubemapFace face, typename PixelType, PixelType pixelValue>
void CheckFaceBottomRightPixel( Tr2ImageHandler& handler, const Tr2BitmapDimensions& dimensions )
{
	size_t offset = ( dimensions.GetWidth() * dimensions.GetHeight() ) * GetBytesPerPixel( dimensions.GetFormat() ) - sizeof( PixelType );
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( handler.GetMipBytes( 0, face ) + offset )[0] );
}

template <uint32_t mip, typename PixelType, PixelType pixelValue>
void CheckMipTopLeftPixel( Tr2ImageHandler& handler, const Tr2BitmapDimensions& dimensions )
{
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( handler.GetMipBytes( mip ) )[0] );
}

template <uint32_t mip, typename PixelType, PixelType pixelValue>
void CheckMipBottomRightPixel( Tr2ImageHandler& handler, const Tr2BitmapDimensions& dimensions )
{
	size_t offset = ( handler.GetMipLevelWidth( mip ) * dimensions.GetMipHeight( mip ) * dimensions.GetMipDepth( mip ) ) * GetBytesPerPixel( dimensions.GetFormat() ) - sizeof( PixelType );
	EXPECT_EQ( pixelValue, reinterpret_cast<const PixelType*>( handler.GetMipBytes( mip ) + offset )[0] );
}

struct TestImage
{
	const wchar_t* fileNameWide;
	uint8_t* data;
	size_t dataSize;
	Tr2BitmapDimensions dimensions;
	GetPixelDataFunction imageChecks[16];
};

void TestReadImage( const TestImage& image );
void AssertReadImageFails( const TestImage& image );
void AssertReadTruncatedImageFails( const TestImage& image, size_t truncatedSize );

void AssertCanSaveImage( const TestImage& image );
void AssertCanSaveImageContentsMayDiffer( const TestImage& image );

#endif