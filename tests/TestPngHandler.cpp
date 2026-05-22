// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "TestHelpers.h"

using namespace ImageIO;

namespace
{

const TestImage s_r = { 
	TEST_FILE( r, png ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8_UNORM, 7, 5, 1, 1 ), 
	&CheckTopLeftPixel<uint8_t, 0x54>, &CheckBottomRightPixel<uint8_t, 0x22> };
const TestImage s_rg = { 
	TEST_FILE( rg, png ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8G8_UNORM, 7, 4, 1, 1 ), 
	&CheckTopLeftPixel<uint16_t, 0xffd3>, &CheckBottomRightPixel<uint16_t, 0xff30> };
const TestImage s_rgb = { 
	TEST_FILE( rgb, png ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 8, 3, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xffb12401>, &CheckBottomRightPixel<uint32_t, 0xff003600> };
const TestImage s_rgba = { 
	TEST_FILE( rgba, png ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 5, 12, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0x72627f00>, &CheckBottomRightPixel<uint32_t, 0x02150000> };
const TestImage s_iDontExist = { L"resources/iDontExist.png", nullptr, 0 };

}

TEST( PngHandler, CanLoadRPng )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_r );
}

TEST( PngHandler, CanLoadRgPng )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rg );
}

TEST( PngHandler, CanLoadRgbPng )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgb );
}

TEST( PngHandler, CanLoadRgbaPng )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgba );
}

TEST( PngHandler, LoadingPngWithTrimmedHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgba, 25 );
}

TEST( PngHandler, LoadingPpgWithTrimmedDataFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgb, 160 );
}

TEST( PngHandler, LoadingNonExistentPngFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadImageFails( s_iDontExist );
}

TEST( PngHandler, CanSaveRPng )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_r );
}

TEST( PngHandler, DISABLED_CanSaveRgPng )
{
	// RG saving doesn't work yet
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rg );
}

TEST( PngHandler, CanSaveRgbPng )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgb );
}

TEST( PngHandler, CanSaveRgbaPng )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgba );
}
