// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "TestHelpers.h"

using namespace ImageIO;

namespace
{

const TestImage s_rgb = { 
	TEST_FILE( rgb, jpg ),
	BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 7, 3, 1, 1 ), 
	&CheckTopLeftPixel<uint32_t, 0xff005305>,&CheckBottomRightPixel<uint32_t,  0xff000c23>, nullptr };
const TestImage s_iDontExist = { L"resources/iDontExist.jpg", nullptr, 0 };

}

TEST( JpgHandler, CanLoadJpg )
{
	SCOPED_TRACE( __FUNCTION__ );
	TestReadImage( s_rgb );
}

TEST( JpgHandler, LoadingJpgWithTrimmedHeaderFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgb, 50 );
}

TEST( JpgHandler, LoadingJpgWithTrimmedDataFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadTruncatedImageFails( s_rgb, 17350 );
}

TEST( JpgHandler, LoadingNonExistentJpgFails )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertReadImageFails( s_iDontExist );
}

TEST( JpgHandler, CanSaveJpg )
{
	SCOPED_TRACE( __FUNCTION__ );
	AssertCanSaveImageContentsMayDiffer( s_rgb );
}
