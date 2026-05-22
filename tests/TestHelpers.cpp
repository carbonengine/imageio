// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "TestHelpers.h"
#include "MemoryStream.h"

namespace
{

void AssertBitmapDimensionsEqual( const ImageIO::BitmapDimensions& actualDimensions, const ImageIO::BitmapDimensions& expectedDimensions )
{
	EXPECT_EQ( expectedDimensions.GetWidth(), actualDimensions.GetWidth() );
	EXPECT_EQ( expectedDimensions.GetHeight(), actualDimensions.GetHeight() );
	EXPECT_EQ( expectedDimensions.GetDepth(), actualDimensions.GetDepth() );
	EXPECT_EQ( expectedDimensions.GetFormat(), actualDimensions.GetFormat() );
	EXPECT_EQ( expectedDimensions.GetType(), actualDimensions.GetType() );
	EXPECT_FALSE( IsCompressedFormat( actualDimensions.GetFormat() ) );
}

void SaveAndLoadImage( const TestImage& image, ImageIO::HostBitmap& loaded, ImageIO::HostBitmap& loadedFromSaved )
{
	ReadMemoryStream stream( image.data, image.dataSize );
	ImageIO::HostBitmap bitmap;
	ASSERT_TRUE( ImageIO::ReadImage( stream, ImageIO::LoadParameters( image.fileNameWide ), loaded ) );
	ASSERT_TRUE( loaded.IsValid() );

	WriteMemoryStream outStream;
	ASSERT_TRUE( ImageIO::SaveImage( image.fileNameWide, loaded, outStream ) );

	ReadMemoryStream stream2( outStream.GetData(), outStream.GetDataSize() );
	ASSERT_TRUE( ImageIO::ReadImage( stream2, ImageIO::LoadParameters( image.fileNameWide ), loadedFromSaved ) );
	ASSERT_TRUE( loadedFromSaved.IsValid() );
	AssertBitmapDimensionsEqual( loadedFromSaved, image.dimensions );
}

}

void TestReadImage( const TestImage& image )
{
	ReadMemoryStream stream( image.data, image.dataSize );
	ImageIO::HostBitmap loaded;
	ASSERT_TRUE( ImageIO::ReadImage( stream, ImageIO::LoadParameters( image.fileNameWide ), loaded ) );
	ASSERT_TRUE( loaded.IsValid() );
	AssertBitmapDimensionsEqual( loaded, image.dimensions );
	if( ::testing::Test::HasFatalFailure() )
	{
		return;
	}
	for( int i = 0; i < sizeof( image.imageChecks ) / sizeof( image.imageChecks[0] ); ++i )
	{
		if( !image.imageChecks[i] )
		{
			break;
		}
		( *image.imageChecks[i] )( loaded, image.dimensions );
	}
}

void AssertReadTruncatedImageFails( const TestImage& image, size_t truncateSize )
{
	ReadMemoryStream stream( image.data, truncateSize );
	ImageIO::HostBitmap loaded;
	ASSERT_FALSE( ImageIO::ReadImage( stream, ImageIO::LoadParameters( image.fileNameWide ), loaded ) );
	ASSERT_FALSE( loaded.IsValid() );
}

void AssertReadImageFails( const TestImage& image )
{
	ReadMemoryStream stream( image.data, image.dataSize );
	ImageIO::HostBitmap loaded;
	ASSERT_FALSE( ImageIO::ReadImage( stream, ImageIO::LoadParameters( image.fileNameWide ), loaded ) );
	ASSERT_FALSE( loaded.IsValid() );
}

void AssertCanSaveImage( const TestImage& image )
{
	ImageIO::HostBitmap bmp1, bmp2;
	SaveAndLoadImage( image, bmp1, bmp2 );
	if( ::testing::Test::HasFatalFailure() )
	{
		return;
	}

	ASSERT_EQ( bmp1.GetRawDataSize(), bmp2.GetRawDataSize() );
	ASSERT_EQ( 0, memcmp( bmp1.GetRawData(), bmp2.GetRawData(), bmp1.GetRawDataSize() ) );
}

void AssertCanSaveImageContentsMayDiffer( const TestImage& image )
{
	ImageIO::HostBitmap bmp1, bmp2;
	SaveAndLoadImage( image, bmp1, bmp2 );
}