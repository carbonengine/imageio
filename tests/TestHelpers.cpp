#include "StdAfx.h"
#include "TestHelpers.h"
#include "MemoryStream.h"

namespace
{

void AssertBitmapDimensionsEqual( Tr2ImageHandler* handler, const Tr2BitmapDimensions& dimensions )
{
	EXPECT_EQ( dimensions.GetWidth(), handler->GetWidth() );
	EXPECT_EQ( dimensions.GetHeight(), handler->GetHeight() );
	EXPECT_EQ( dimensions.GetDepth(), handler->GetDepth() );
	EXPECT_EQ( dimensions.GetFormat(), handler->GetFormat() );
	switch( dimensions.GetType() )
	{
	case Tr2RenderContextEnum::TEX_TYPE_2D:
		EXPECT_FALSE( handler->IsCubeTexture() );
		EXPECT_FALSE( handler->IsVolumeTexture() );
		break;
	case Tr2RenderContextEnum::TEX_TYPE_3D:
		EXPECT_FALSE( handler->IsCubeTexture() );
		EXPECT_TRUE( handler->IsVolumeTexture() );
		break;
	case Tr2RenderContextEnum::TEX_TYPE_CUBE:
		EXPECT_TRUE( handler->IsCubeTexture() );
		EXPECT_FALSE( handler->IsVolumeTexture() );
		break;
	}
	EXPECT_FALSE( handler->IsCompressed() );
	EXPECT_EQ( 0, handler->GetBlockByteSize() );
}

void SaveAndLoadImage( const TestImage& image, ImageIO::HostBitmap& loaded, ImageIO::HostBitmap& loadedFromSaved )
{
	ReadMemoryStream stream( image.data, image.dataSize );
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( image.fileNameWide ) );
	ASSERT_TRUE( handler->ReadHeader( &stream ) );
	ASSERT_TRUE( handler->ReadImage( &stream ) );

	ASSERT_TRUE( loaded.CreateFromImageHandler( handler.get() ) );

	WriteMemoryStream outStream;
	std::unique_ptr<Tr2ImageHandler> outHandler( CreateImageHandler( image.fileNameWide ) );
	ASSERT_TRUE( outHandler->Save( loaded, &outStream ) );

	ReadMemoryStream stream2( outStream.GetData(), outStream.GetDataSize() );
	std::unique_ptr<Tr2ImageHandler> handler2( CreateImageHandler( image.fileNameWide ) );
	ASSERT_TRUE( handler2->ReadHeader( &stream2 ) );
	ASSERT_TRUE( handler2->ReadImage( &stream2 ) );
	AssertBitmapDimensionsEqual( handler2.get(), image.dimensions );
	if( ::testing::Test::HasFatalFailure() )
	{
		return;
	}

	ASSERT_TRUE( loadedFromSaved.CreateFromImageHandler( handler2.get() ) );
}

}

void TestReadImage( const TestImage& image )
{
	ReadMemoryStream stream( image.data, image.dataSize );
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( image.fileNameWide ) );
	ASSERT_TRUE( handler->ReadHeader( &stream ) );
	ASSERT_TRUE( handler->ReadImage( &stream ) );
	AssertBitmapDimensionsEqual( handler.get(), image.dimensions );
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
		( *image.imageChecks[i] )( *handler, image.dimensions );
	}
}

void AssertReadTruncatedImageFails( const TestImage& image, size_t truncateSize )
{
	ReadMemoryStream stream( image.data, truncateSize );
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( image.fileNameWide ) );
	if( !handler->ReadHeader( &stream ) )
	{
		return;
	}
	ASSERT_FALSE( handler->ReadImage( &stream ) );
}

void AssertReadImageFails( const TestImage& image )
{
	ReadMemoryStream stream( image.data, image.dataSize );
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( image.fileNameWide ) );
	if( !handler->ReadHeader( &stream ) )
	{
		return;
	}
	ASSERT_FALSE( handler->ReadImage( &stream ) );
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