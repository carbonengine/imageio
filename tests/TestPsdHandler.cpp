// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "TestHelpers.h"

using namespace ImageIO;

namespace
{

const TestImage s_rgb = { 
	TEST_FILE( rgb, psd ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 5, 12, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xff000000>, &CheckBottomRightPixel<uint32_t, 0xff100000>, nullptr };
const TestImage s_rgba = { 
	TEST_FILE( rgba, psd ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 5, 12, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0x79000000>, &CheckBottomRightPixel<uint32_t, 0xd1100000>, nullptr };
const TestImage s_al = { 
	TEST_FILE( al, psd ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8G8_UNORM, 5, 12, 1, 1 ), 
	&CheckTopLeftPixel<uint16_t, 0x7900>, &CheckBottomRightPixel<uint16_t, 0xd104>, nullptr };
const TestImage s_r = { 
	TEST_FILE( r, psd ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8_UNORM, 5, 12, 1, 1 ), 
	&CheckTopLeftPixel<uint8_t, 0x00>, &CheckBottomRightPixel<uint8_t, 0x04>, nullptr };
const TestImage s_rgbRle = { 
	TEST_FILE( rgbRle, psd ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 32, 32, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xff636161>, &CheckBottomRightPixel<uint32_t, 0xffbfbfbf>, nullptr };
const TestImage s_iDontExist = { L"resources/iDontExist.psd", nullptr, 0 };

}

TEST( PsdHandler, CanLoadRgbPsd )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgb );
}

TEST( PsdHandler, CanLoadRgbaPsd )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgba );
}

TEST( PsdHandler, CanLoadGrayscaleWithAlphaPsd )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_al );
}

TEST( PsdHandler, CanLoadGrayscalePsd )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_r );
}

TEST( PsdHandler, CanLoadRgbPsdWithRleCompression )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgbRle );
}

TEST( PsdHandler, LoadingRgbPsdWithTrimmedHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgb, 13 );
}

TEST( PsdHandler, LoadingRgbPsdWithTrimmedDataFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgb, s_rgb.dataSize - 20 );
}

TEST( PsdHandler, LoadingRgbRlePsdWithTrimmedHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgbRle, 13 );
}

TEST( PsdHandler, LoadingRgbPsdRleWithTrimmedDataFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgbRle, s_rgbRle.dataSize - 20 );
}

TEST( PsdHandler, LoadingNonExistentPsdFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadImageFails( s_iDontExist );
}

TEST( PsdHandler, CanSaveRgbPsd )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgb );
}

TEST( PsdHandler, CanSaveRgbaPsd )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgba );
}

TEST( PsdHandler, CanSaveGrayscaleWithAlphaPsd )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_al );
}

TEST( PsdHandler, CanSaveGrayscalePsd )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_r );
}
