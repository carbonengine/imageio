// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "TestHelpers.h"

using namespace ImageIO;

namespace
{

const TestImage s_rgb = { 
	TEST_FILE( rgb, bmp ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 7, 3, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xff005200>, &CheckBottomRightPixel<uint32_t, 0xff000039>, nullptr };
const TestImage s_rgba = { 
	TEST_FILE( rgba, bmp ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 5, 4, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xd9f953a4>, &CheckBottomRightPixel<uint32_t, 0x2a630000>, nullptr };
const TestImage s_rgbGarbled = { TEST_FILE( rgbGarbled, bmp ) };
const TestImage s_rgbaGarbled = { TEST_FILE( rgbaGarbled, bmp ) };
const TestImage s_iDontExist = { L"resources/iDontExist.bmp", nullptr, 0 };

}

TEST( BmpHandler, CanLoadRgbBmp )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgb );
}

TEST( BmpHandler, CanLoadRgbaBmp )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgba );
}

TEST( BmpHandler, LoadingRgbBmpWithTrimmedHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgb, 41 );
}

TEST( BmpHandler, LoadingRgbBmpWithTrimmedDataFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgb, 100 );
}

TEST( BmpHandler, LoadingRgbBmpWithGarbledHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadImageFails( s_rgbGarbled );
}

TEST( BmpHandler, LoadingRgbaBmpWithTrimmedHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgba, 24 );
}

TEST( BmpHandler, LoadingRgbaBmpWithTrimmedDataFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgba, 101 );
}

TEST( BmpHandler, LoadingRgbaBmpWithGarbledHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadImageFails( s_rgbaGarbled );
}

TEST( BmpHandler, LoadingNonExistentBmpFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadImageFails( s_iDontExist );
}

TEST( BmpHandler, CanSaveRgbBmp )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgb );
}

TEST( BmpHandler, CanSaveRgbaBmp )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgba );
}
