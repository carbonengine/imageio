// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "TestHelpers.h"

using namespace ImageIO;

namespace
{

const TestImage s_rgb = { 
	TEST_FILE( rgb, dds ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 5, 12, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0x00000000>, &CheckBottomRightPixel<uint32_t, 0x00120000>, nullptr };
const TestImage s_rgba = { 
	TEST_FILE( rgba, dds ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 5, 12, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xf7000000>, &CheckBottomRightPixel<uint32_t, 0x74120000>, nullptr };
const TestImage s_rgbx = { 
	TEST_FILE( rgbx, dds ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 5, 12, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0x00000000>, &CheckBottomRightPixel<uint32_t, 0x00120000>, nullptr };
const TestImage s_a = { 
	TEST_FILE( a, dds ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_A8_UNORM, 7, 6, 1, 1 ), 
	&CheckTopLeftPixel<uint8_t, 0xa3>, &CheckBottomRightPixel<uint8_t, 0x00>, nullptr };
const TestImage s_l = { 
	TEST_FILE( l, dds ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R8_UNORM, 7, 6, 1, 1 ), 
	&CheckTopLeftPixel<uint8_t, 0xb4>, &CheckBottomRightPixel<uint8_t, 0x00>, nullptr };
const TestImage s_al = { 
	TEST_FILE( al, dds ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 7, 6, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xa3696969>, &CheckBottomRightPixel<uint32_t, 0x00262626>, nullptr };
const TestImage s_rgba16f = { 
	TEST_FILE( rgba16f, dds ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R16G16B16A16_FLOAT, 4, 7, 1, 1 ), 
	&CheckTopLeftPixel<uint64_t, 0x0000000000000000>, &CheckBottomRightPixel<uint64_t, 0x3a250000000034ef>, nullptr };
const TestImage s_rgba16 = { 
	TEST_FILE( rgba16, dds ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_R16G16B16A16_UNORM, 16, 15, 1, 1 ), 
	&CheckTopLeftPixel<uint64_t, 0xdd07000000000000>, &CheckBottomRightPixel<uint64_t, 0xdd07182e177a173e>, nullptr };
const TestImage s_rgbCube = { 
	TEST_FILE( rgbCube, dds ),
	BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8X8_UNORM, 5, 5, 1, 1 ),
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_POSITIVE_X, uint32_t, 0xb44100>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_POSITIVE_X, uint32_t, 0xb80000>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_NEGATIVE_X, uint32_t, 0x000009>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_NEGATIVE_X, uint32_t, 0x000000>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_POSITIVE_Y, uint32_t, 0xd70074>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_POSITIVE_Y, uint32_t, 0x004300>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_NEGATIVE_Y, uint32_t, 0x0021aa>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_NEGATIVE_Y, uint32_t, 0x2cc800>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_POSITIVE_Z, uint32_t, 0x1470ad>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_POSITIVE_Z, uint32_t, 0xf700eb>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_NEGATIVE_Z, uint32_t, 0xdb0000>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_NEGATIVE_Z, uint32_t, 0x834000>,
	nullptr };
const TestImage s_rgbaCube = { 
	TEST_FILE( rgbaCube, dds ),
	BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8A8_UNORM, 5, 5, 1, 1 ),
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_POSITIVE_X, uint32_t, 0x6bb44100>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_POSITIVE_X, uint32_t, 0x00b80000>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_NEGATIVE_X, uint32_t, 0x00000009>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_NEGATIVE_X, uint32_t, 0xd5000000>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_POSITIVE_Y, uint32_t, 0x00d70074>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_POSITIVE_Y, uint32_t, 0x00004300>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_NEGATIVE_Y, uint32_t, 0x7c0021aa>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_NEGATIVE_Y, uint32_t, 0x002cc800>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_POSITIVE_Z, uint32_t, 0x041470ad>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_POSITIVE_Z, uint32_t, 0x25f700eb>, 
	&CheckFaceTopLeftPixel<CUBEMAP_FACE_NEGATIVE_Z, uint32_t, 0x00db0000>, 
	&CheckFaceBottomRightPixel<CUBEMAP_FACE_NEGATIVE_Z, uint32_t, 0x00834000>,
	nullptr };
const TestImage s_rgbaVolume = { 
	TEST_FILE( rgbaVolume, dds ),
	BitmapDimensions( TEX_TYPE_3D, PIXEL_FORMAT_B8G8R8A8_UNORM, 4, 4, 4, 1 ),
	&CheckTopLeftPixel<uint32_t, 0x0000c79c>, &CheckBottomRightPixel<uint32_t, 0x55bd54e2>, nullptr };
const TestImage s_rgbaMips = { 
	TEST_FILE( rgbaMips, dds ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 24, 27, 1, 3 ),
	&CheckTopLeftPixel<uint32_t, 0xff0000d3>, 
	&CheckBottomRightPixel<uint32_t, 0x37b200ff>, 
	&CheckMipTopLeftPixel<1, uint32_t, 0x6d2c1031>, 
	&CheckMipBottomRightPixel<1, uint32_t, 0x183c6845>, 
	&CheckMipTopLeftPixel<2, uint32_t, 0x4e502f44>, 
	&CheckMipBottomRightPixel<2, uint32_t, 0x3c425630>, 
	nullptr };


}

TEST( DdsHandler, CanLoadRgbDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgb );
}

TEST( DdsHandler, CanLoadRgbaDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgba );
}

TEST( DdsHandler, CanLoadRgbxDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgbx );
}

TEST( DdsHandler, CanLoadADds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_a );
}

TEST( DdsHandler, CanLoadLDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_l );
}

TEST( DdsHandler, CanLoadAlDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_al );
}

TEST( DdsHandler, CanLoadRgba16fDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgba16f );
}

TEST( DdsHandler, CanLoadRgba16Dds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgba16 );
}

TEST( DdsHandler, CanLoadRgbCubeDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgbCube );
}

TEST( DdsHandler, CanLoadRgbaCubeDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgbaCube );
}

TEST( DdsHandler, CanLoadRgbaVolumeDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgbaVolume );
}

TEST( DdsHandler, CanLoadRgbaWithMipsDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgbaMips );
}

TEST( DdsHandler, CanSaveRgbDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgb );
}

TEST( DdsHandler, CanSaveRgbaDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgba );
}

TEST( DdsHandler, CanSaveRgbxDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgbx );
}

TEST( DdsHandler, CanSaveADds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_a );
}

TEST( DdsHandler, CanSaveLDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_l );
}

TEST( DdsHandler, CanSaveAlDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_al );
}

TEST( DdsHandler, CanSaveRgba16Dds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgba16 );
}

TEST( DdsHandler, CanSaveRgba16fDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgba16f );
}

TEST( DdsHandler, CanSaveRgbCubeDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgbCube );
}

TEST( DdsHandler, CanSaveRgbaCubeDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgbaCube );
}

TEST( DdsHandler, CanSaveRgbaVolumeDds )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgbaVolume );
}

TEST( DdsHandler, CanSaveRgbaDdsWithMips )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImage( s_rgbaMips );
}
