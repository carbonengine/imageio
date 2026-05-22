// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "TestHelpers.h"

using namespace ImageIO;

namespace
{

const TestImage s_rgb = { 
	TEST_FILE( rgb, tga ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 5, 8, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xffff2aff>, &CheckBottomRightPixel<uint32_t, 0xffff00b9>, nullptr };
const TestImage s_rgbRle = { 
	TEST_FILE( rgbRle, tga ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 5, 8, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xffff2aff>, &CheckBottomRightPixel<uint32_t, 0xffff00b9>, nullptr };
const TestImage s_rgba = { 
	TEST_FILE( rgba, tga ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 5, 8, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xffff2aff>, &CheckBottomRightPixel<uint32_t, 0x61ff00b9>, nullptr };
const TestImage s_rgbaRle = { 
	TEST_FILE( rgbaRle, tga ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 5, 8, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xffff2aff>, &CheckBottomRightPixel<uint32_t, 0x61ff00b9>, nullptr };
const TestImage s_r = { 
	TEST_FILE( r, tga ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8_UNORM, 5, 8, 1, 1 ), 
	&CheckTopLeftPixel<uint8_t, 0xff>, &CheckBottomRightPixel<uint8_t, 0x61>, nullptr };
const TestImage s_rgbIndexed = { 
	TEST_FILE( rgbIndexed, tga ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 5, 8, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xffff2aff>, &CheckBottomRightPixel<uint32_t, 0xffff00b9>, nullptr };
const TestImage s_iDontExist = { L"resources/iDontExist.tga", nullptr, 0 };

}

TEST( TgaHandler, CanLoadRgbTga )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgb );
}

TEST( TgaHandler, CanLoadRgbTgaWithRleCompression )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgbRle );
}

TEST( TgaHandler, CanLoadRgbaTga )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgba );
}

TEST( TgaHandler, CanLoadRgbaTgaWithRleCompression )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgbaRle );
}

TEST( TgaHandler, CanLoadGrayscaleTga )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_r );
}

TEST( TgaHandler, CanLoadRgbTgaWithWithPalette )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgbIndexed );
}

TEST( TgaHandler, LoadingRgbTgaWithTrimmedHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgb, 12 );
}

TEST( TgaHandler, LoadingRgbTgaWithTrimmedDataFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgb, 100 );
}

TEST( TgaHandler, LoadingRgbaRleTgaWithTrimmedHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgbaRle, 12 );
}

TEST( TgaHandler, LoadingRgbaRleTgaWithTrimmedDataFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgbaRle, 100 );
}

TEST( TgaHandler, LoadingNonExistentTgaFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadImageFails( s_iDontExist );
}

TEST( TgaHandler, CanSaveRgbTga )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgb );
}

TEST( TgaHandler, CanSaveRgbaTga )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgba );
}

TEST( TgaHandler, CanSaveGrayscaleTga )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_r );
}
