// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "TestHelpers.h"
#include "MemoryStream.h"

using namespace ImageIO;
namespace {

	bool LoadBitmap( TestImage testImage, ImageIO::HostBitmap &bitmap )
	{
		ReadMemoryStream stream( testImage.data, testImage.dataSize );
		return ImageIO::ReadImage( stream, ImageIO::LoadParameters( testImage.fileNameWide ), bitmap );
	}

	const TestImage s_rgbx = {
		TEST_FILE( rgbx, dds ),
		BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8X8_UNORM, 5, 12, 1, 1 ), nullptr };

	const TestImage s_rgba = {
		TEST_FILE( rgba, dds ),
		BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_B8G8R8A8_UNORM, 5, 12, 1, 1 ), nullptr };

	const TestImage s_bc1 = {
		TEST_FILE( bc1, dds ),
		BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_BC1_UNORM, 8, 8, 1, 4 ), nullptr };

	const TestImage s_bc3 = {
		TEST_FILE( bc3, dds ),
		BitmapDimensions( TEX_TYPE_2D, PIXEL_FORMAT_BC3_UNORM, 32, 32, 1, 5 ), nullptr };

}


TEST( HostBitmap, BitmapIsInvalidBeforeCreation )
{
	HostBitmap bmp;
	ASSERT_FALSE( bmp.IsValid() );
}

TEST( HostBitmap, CanCreate2DBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 32, 16, 1, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( TEX_TYPE_2D, bmp.GetType() );
	EXPECT_EQ( 32, bmp.GetWidth() );
	EXPECT_EQ( 16, bmp.GetHeight() );
	EXPECT_EQ( 1, bmp.GetDepth() );
	EXPECT_EQ( 1, bmp.GetMipCount() );
	EXPECT_EQ( 32 * 16, bmp.GetRawDataSize() );
	EXPECT_NE( nullptr, bmp.GetRawData() );
	EXPECT_EQ( bmp.GetMipRawData( 0 ), bmp.GetRawData() );
}

TEST( HostBitmap, CanCreate2DBitmapArray )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create2DArray( 32, 16, 1, 3, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( TEX_TYPE_2D, bmp.GetType() );
	EXPECT_EQ( 32, bmp.GetWidth() );
	EXPECT_EQ( 16, bmp.GetHeight() );
	EXPECT_EQ( 1, bmp.GetDepth() );
	EXPECT_EQ( 1, bmp.GetMipCount() );
	EXPECT_EQ( 3, bmp.GetArraySize() );
	EXPECT_EQ( 32 * 16 * 3, bmp.GetRawDataSize() );
	EXPECT_EQ( 32 * 16, bmp.GetArrayElementSize() );
	EXPECT_NE( nullptr, bmp.GetRawData() );
	EXPECT_EQ( bmp.GetMipRawData( 0 ), bmp.GetRawData() );
}

TEST( HostBitmap, BitmapIsInvalidAfterDestruction )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 32, 16, 1, PIXEL_FORMAT_A8_UNORM ) );
	bmp.Destroy();
	ASSERT_FALSE( bmp.IsValid() );
}

TEST( HostBitmap, CanCreateCubeBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateCube( 32, 1, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( TEX_TYPE_CUBE, bmp.GetType() );
	EXPECT_EQ( 32, bmp.GetWidth() );
	EXPECT_EQ( 32, bmp.GetHeight() );
	EXPECT_EQ( 1, bmp.GetDepth() );
	EXPECT_EQ( 1, bmp.GetMipCount() );
	EXPECT_EQ( 32 * 32 * 6, bmp.GetRawDataSize() );
	EXPECT_NE( nullptr, bmp.GetRawData() );
	EXPECT_EQ( bmp.GetMipRawData( 0 ), bmp.GetRawData() );
}

TEST( HostBitmap, CanCreateVolumeBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateVolume( 32, 16, 8, 1, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( TEX_TYPE_3D, bmp.GetType() );
	EXPECT_EQ( 32, bmp.GetWidth() );
	EXPECT_EQ( 16, bmp.GetHeight() );
	EXPECT_EQ( 8, bmp.GetDepth() );
	EXPECT_EQ( 1, bmp.GetMipCount() );
	EXPECT_EQ( 32 * 16 * 8, bmp.GetRawDataSize() );
	EXPECT_NE( nullptr, bmp.GetRawData() );
	EXPECT_EQ( bmp.GetMipRawData( 0 ), bmp.GetRawData() );
}

TEST( HostBitmap, CanCreate2DBitmapWithMips )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 32, 16, 2, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( 16, bmp.GetMipWidth( 1 ) );
	EXPECT_EQ( 8, bmp.GetMipHeight( 1 ) );
	EXPECT_EQ( 2, bmp.GetMipCount() );
	EXPECT_EQ( 32 * 16 + 16 * 8, bmp.GetRawDataSize() );
	EXPECT_EQ( 32 * 16, bmp.GetMipRawData( 1 ) - bmp.GetMipRawData( 0 ) );
}

TEST( HostBitmap, CanCreateNpot2DBitmapWithMips )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 35, 16, 2, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( 17, bmp.GetMipWidth( 1 ) );
	EXPECT_EQ( 8, bmp.GetMipHeight( 1 ) );
	EXPECT_EQ( 2, bmp.GetMipCount() );
	EXPECT_EQ( 35 * 16 + 17 * 8, bmp.GetRawDataSize() );
	EXPECT_EQ( 35 * 16, bmp.GetMipRawData( 1 ) - bmp.GetMipRawData( 0 ) );
}

TEST( HostBitmap, CanCreate2DBitmapWithFullMipChain )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 33, 13, 0, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( 0, bmp.GetMipCount() );
	EXPECT_EQ( 6, bmp.GetTrueMipCount() );
	EXPECT_EQ( 16, bmp.GetMipWidth( 1 ) );
	EXPECT_EQ( 6, bmp.GetMipHeight( 1 ) );
	EXPECT_EQ( 8, bmp.GetMipWidth( 2 ) );
	EXPECT_EQ( 3, bmp.GetMipHeight( 2 ) );
	EXPECT_EQ( 4, bmp.GetMipWidth( 3 ) );
	EXPECT_EQ( 1, bmp.GetMipHeight( 3 ) );
	EXPECT_EQ( 2, bmp.GetMipWidth( 4 ) );
	EXPECT_EQ( 1, bmp.GetMipHeight( 4 ) );
	EXPECT_EQ( 1, bmp.GetMipWidth( 5 ) );
	EXPECT_EQ( 1, bmp.GetMipHeight( 5 ) );
}

TEST( HostBitmap, CanCreateCubeBitmapWithMips )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateCube( 32, 2, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( 16, bmp.GetMipWidth( 1 ) );
	EXPECT_EQ( 16, bmp.GetMipHeight( 1 ) );
	EXPECT_EQ( 2, bmp.GetMipCount() );
	EXPECT_EQ( 32 * 32 * 6 + 16 * 16 * 6, bmp.GetRawDataSize() );
	EXPECT_EQ( 32 * 32, bmp.GetMipRawData( 1, CUBEMAP_FACE_NEGATIVE_X ) - bmp.GetMipRawData( 0, CUBEMAP_FACE_NEGATIVE_X ) );
	EXPECT_EQ( 32 * 32, bmp.GetMipRawData( 1, CUBEMAP_FACE_POSITIVE_Y ) - bmp.GetMipRawData( 0, CUBEMAP_FACE_POSITIVE_Y ) );
}

TEST( HostBitmap, CanCreateNpotCubeBitmapWithMips )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateCube( 35, 2, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( 17, bmp.GetMipWidth( 1 ) );
	EXPECT_EQ( 17, bmp.GetMipHeight( 1 ) );
	EXPECT_EQ( 2, bmp.GetMipCount() );
	EXPECT_EQ( 35 * 35 * 6 + 17 * 17 * 6, bmp.GetRawDataSize() );
	EXPECT_EQ( 35 * 35, bmp.GetMipRawData( 1, CUBEMAP_FACE_NEGATIVE_X ) - bmp.GetMipRawData( 0, CUBEMAP_FACE_NEGATIVE_X ) );
}

TEST( HostBitmap, CanCreateCubeBitmapWithFullMipChain )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 11, 11, 0, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( 0, bmp.GetMipCount() );
	EXPECT_EQ( 4, bmp.GetTrueMipCount() );
	EXPECT_EQ( 5, bmp.GetMipWidth( 1 ) );
	EXPECT_EQ( 5, bmp.GetMipHeight( 1 ) );
	EXPECT_EQ( 2, bmp.GetMipWidth( 2 ) );
	EXPECT_EQ( 2, bmp.GetMipHeight( 2 ) );
	EXPECT_EQ( 1, bmp.GetMipWidth( 3 ) );
	EXPECT_EQ( 1, bmp.GetMipHeight( 3 ) );
}

TEST( HostBitmap, CanCreateVolumeBitmapWithMips )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateVolume( 32, 16, 8, 2, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( 16, bmp.GetMipWidth( 1 ) );
	EXPECT_EQ( 8, bmp.GetMipHeight( 1 ) );
	EXPECT_EQ( 4, bmp.GetMipDepth( 1 ) );
	EXPECT_EQ( 2, bmp.GetMipCount() );
	EXPECT_EQ( 32 * 16 * 8 + 16 * 8 * 4, bmp.GetRawDataSize() );
	EXPECT_EQ( 32 * 16 * 8, bmp.GetMipRawData( 1 ) - bmp.GetMipRawData( 0 ) );
}

TEST( HostBitmap, CanCreateNpotVolumeBitmapWithMips )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateVolume( 33, 11, 7, 2, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( 16, bmp.GetMipWidth( 1 ) );
	EXPECT_EQ( 5, bmp.GetMipHeight( 1 ) );
	EXPECT_EQ( 3, bmp.GetMipDepth( 1 ) );
	EXPECT_EQ( 2, bmp.GetMipCount() );
	EXPECT_EQ( 33 * 11 * 7 + 16 * 5 * 3, bmp.GetRawDataSize() );
	EXPECT_EQ( 33 * 11 * 7, bmp.GetMipRawData( 1 ) - bmp.GetMipRawData( 0 ) );
}

TEST( HostBitmap, CanCreateVolumeBitmapWithFullMipChain )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateVolume( 31, 11, 7, 0, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_EQ( 0, bmp.GetMipCount() );
	EXPECT_EQ( 5, bmp.GetTrueMipCount() );

	EXPECT_EQ( 15, bmp.GetMipWidth( 1 ) );
	EXPECT_EQ( 5, bmp.GetMipHeight( 1 ) );
	EXPECT_EQ( 3, bmp.GetMipDepth( 1 ) );
	EXPECT_EQ( 7, bmp.GetMipWidth( 2 ) );
	EXPECT_EQ( 2, bmp.GetMipHeight( 2 ) );
	EXPECT_EQ( 1, bmp.GetMipDepth( 2 ) );
	EXPECT_EQ( 3, bmp.GetMipWidth( 3 ) );
	EXPECT_EQ( 1, bmp.GetMipHeight( 3 ) );
	EXPECT_EQ( 1, bmp.GetMipDepth( 3 ) );
	EXPECT_EQ( 1, bmp.GetMipWidth( 4 ) );
	EXPECT_EQ( 1, bmp.GetMipHeight( 4 ) );
	EXPECT_EQ( 1, bmp.GetMipDepth( 4 ) );
}


class PixelFormatTest: public ::testing::TestWithParam<int>
{
};

TEST_P( PixelFormatTest, CanGetPitchForBitmap )
{
	PixelFormat format = PixelFormat( GetParam() );
	if( IsCompressedFormat( format ) )
	{
		HostBitmap bmp;
		ASSERT_TRUE( bmp.Create( 64, 8, 1, format ) );
		EXPECT_EQ( GetBlockByteSize( format ) * 64 / 4, bmp.GetPitch() );
	}
	else if( GetBytesPerPixel( format ) )
	{
		HostBitmap bmp;
		ASSERT_TRUE( bmp.Create( 64, 8, 1, format ) );
		EXPECT_EQ( GetBytesPerPixel( format ) * 64, bmp.GetPitch() );
	}
}

TEST_P( PixelFormatTest, GetPixelDataHasCorrectOffset )
{
	PixelFormat format = PixelFormat( GetParam() );
	if( !IsCompressedFormat( format ) && GetBytesPerPixel( format ) )
	{
		HostBitmap bmp;
		ASSERT_TRUE( bmp.Create( 64, 8, 1, format ) );
		EXPECT_EQ( bmp.GetPitch() * 2 + GetBytesPerPixel( format ), bmp.GetRawData( 1, 2 ) - bmp.GetRawData() );
	}
}

TEST( HostBitmap, PopulateMarginFailsForCompressedBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 1, PIXEL_FORMAT_BC1_UNORM ) );
	EXPECT_FALSE( bmp.PopulateMargin( 1 ) );
}

TEST( HostBitmap, PopulateMarginFailsForBitmapWithMips )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 7, 6, 2, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_FALSE( bmp.PopulateMargin( 1 ) );
}

TEST( HostBitmap, PopulateMarginFailsForCubeBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateCube( 8, 1, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_FALSE( bmp.PopulateMargin( 1 ) );
}

TEST( HostBitmap, PopulateMarginFailsForVolumeBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateVolume( 8, 8, 8, 1, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_FALSE( bmp.PopulateMargin( 1 ) );
}

TEST( HostBitmap, CanPopulateMargin )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 7, 6, 1, PIXEL_FORMAT_A8_UNORM ) );

	uint8_t pixels[] = { 
		0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 
		0, 0, 1, 2, 3, 4, 0, 
		0, 0, 5, 6, 7, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 
		9, 0, 0, 0, 0, 0, 0 };
	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );

	ASSERT_TRUE( bmp.PopulateMargin( 2 ) );

	uint8_t expectedPixels[] = { 
		0, 0, 1, 2, 3, 0, 0, 
		0, 0, 1, 2, 3, 0, 0, 
		1, 1, 1, 2, 3, 3, 3, 
		5, 5, 5, 6, 7, 7, 7, 
		0, 0, 5, 6, 7, 0, 0, 
		9, 0, 5, 6, 7, 0, 0 };
	EXPECT_EQ( 0, memcmp( expectedPixels, bmp.GetRawData(), sizeof( expectedPixels ) ) );
}

TEST( HostBitmap, Downsample2x2FailsForCompressedBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 1, PIXEL_FORMAT_BC1_UNORM ) );
	EXPECT_FALSE( bmp.Downsample2x2() );
}

TEST( HostBitmap, Downsample2x2FailsForOddBitmapSize )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 9, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	EXPECT_FALSE( bmp.Downsample2x2() );
}

TEST( HostBitmap, Downsample2x2FailsForVolumeBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateVolume( 8, 8, 8, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	EXPECT_FALSE( bmp.Downsample2x2() );
}

TEST( HostBitmap, CanDownsample2x2Bitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 4, 2, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	uint8_t pixels[] = {
		1, 2, 3, 4,  4, 5, 6, 7,  8, 9, 0, 1,  2, 3, 4, 5, 
		3, 6, 2, 5,  8, 4, 5, 1,  5, 8, 3, 5,  0, 7, 3, 1, 
	};
	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );

	ASSERT_TRUE( bmp.Downsample2x2() );
	EXPECT_EQ( 2, bmp.GetWidth() );
	EXPECT_EQ( 1, bmp.GetHeight() );

	uint8_t expectedPixels[] = {
		4, 4, 4, 4,  3, 6, 2, 3, 
	};
	EXPECT_EQ( 0, memcmp( expectedPixels, bmp.GetRawData(), sizeof( expectedPixels ) ) );
}


TEST( HostBitmap, CropFailsForCompressedBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 1, PIXEL_FORMAT_BC1_UNORM ) );
	EXPECT_FALSE( bmp.Crop( 1, 2, 7, 7 ) );
}

TEST( HostBitmap, CropFailsForBitmapWithMips )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 2, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_FALSE( bmp.Crop( 1, 2, 7, 7 ) );
}

TEST( HostBitmap, CropFailsForCubeBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateCube( 8, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	EXPECT_FALSE( bmp.Crop( 1, 2, 7, 7 ) );
}

TEST( HostBitmap, CropFailsForVolumeBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateVolume( 8, 8, 8, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	EXPECT_FALSE( bmp.Crop( 1, 2, 7, 7 ) );
}

TEST( HostBitmap, CanCropBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 10, 8, 1, PIXEL_FORMAT_A8_UNORM ) );
	uint8_t pixels[] = {
		11, 12, 13, 14, 15, 16, 17, 18, 19, 10,
		21, 22, 23, 24, 25, 26, 27, 28, 29, 20,
		31, 32, 33, 34, 35, 36, 37, 38, 39, 30,
		41, 42, 43, 44, 45, 46, 47, 48, 49, 40,
		51, 52, 53, 54, 55, 56, 57, 58, 59, 50,
		61, 62, 63, 64, 65, 66, 67, 68, 69, 60,
		71, 72, 73, 74, 75, 76, 77, 78, 79, 70,
		81, 82, 83, 84, 85, 86, 87, 88, 89, 80,
	};
	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );

	EXPECT_TRUE( bmp.Crop( 1, 2, 7, 7 ) );
	EXPECT_EQ( 6, bmp.GetWidth() );
	EXPECT_EQ( 5, bmp.GetHeight() );
	uint8_t expectedPixels[] = {
		32, 33, 34, 35, 36, 37,
		42, 43, 44, 45, 46, 47,
		52, 53, 54, 55, 56, 57,
		62, 63, 64, 65, 66, 67,
		72, 73, 74, 75, 76, 77,
	};
	EXPECT_EQ( 0, memcmp( bmp.GetRawData(), expectedPixels, sizeof( expectedPixels ) ) );
}


TEST( HostBitmap, ConvertToVolumeFailsForCubeBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateCube( 8, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	EXPECT_FALSE( bmp.ConvertToVolume() );
}

TEST( HostBitmap, ConvertToVolumeFailsForVolumeBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.CreateVolume( 8, 8, 8, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	EXPECT_FALSE( bmp.ConvertToVolume() );
}

TEST( HostBitmap, CanConvertToVolume )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 16, 8, 1, PIXEL_FORMAT_A8_UNORM ) );
	const uint32_t volumeSize = uint32_t( sqrt( float( bmp.GetWidth() ) ) );

	EXPECT_TRUE( bmp.ConvertToVolume() );

	EXPECT_EQ( volumeSize, bmp.GetWidth() );
	EXPECT_EQ( volumeSize, bmp.GetHeight() );
	EXPECT_EQ( volumeSize, bmp.GetDepth() );
}

TEST( HostBitmap, ChangeFormatFailsWhenChangingBetweenCompressedAndUncompressed )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 1, PIXEL_FORMAT_BC1_UNORM ) );
	EXPECT_FALSE( bmp.ChangeFormat( PIXEL_FORMAT_A8_UNORM ) );
}

TEST( HostBitmap, ChangeFormatFailsWhenChangingBetweenUncompressedAndCompressed )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 1, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_FALSE( bmp.ChangeFormat( PIXEL_FORMAT_BC1_UNORM ) );
}

TEST( HostBitmap, ChangeFormatFailsWhenChangingBitsPerPixel )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 1, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_FALSE( bmp.ChangeFormat( PIXEL_FORMAT_B8G8R8A8_UNORM ) );
}

TEST( HostBitmap, CanChangeFormat )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 1, PIXEL_FORMAT_B8G8R8X8_UNORM ) );
	EXPECT_TRUE( bmp.ChangeFormat( PIXEL_FORMAT_B8G8R8A8_UNORM ) );
}

TEST( HostBitmap, CanConvertB8G8R8X8ToB8G8R8A8 )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 0, PIXEL_FORMAT_B8G8R8X8_UNORM ) );
	std::unique_ptr<uint8_t[]> pixels( new uint8_t[bmp.GetRawDataSize()] );
	for( size_t i = 0; i < bmp.GetRawDataSize(); ++i )
	{
		pixels[i] = rand() & 0xff;
	}
	memcpy( bmp.GetRawData(), pixels.get(), bmp.GetRawDataSize() );

	EXPECT_TRUE( bmp.ConvertFormat( PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	for( size_t i = 0; i < bmp.GetRawDataSize(); ++i )
	{
		if( i % 4 == 3 )
		{
			EXPECT_EQ( 0xff, reinterpret_cast<uint8_t*>( bmp.GetRawData() )[i] );
		}
		else
		{
			EXPECT_EQ( pixels[i], reinterpret_cast<uint8_t*>( bmp.GetRawData() )[i] );
		}
	}
}

TEST( HostBitmap, CanConvertL8ToB8G8R8A8 )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 0, PIXEL_FORMAT_R8_UNORM ) );
	std::unique_ptr<uint8_t[]> pixels( new uint8_t[bmp.GetRawDataSize()] );
	for( size_t i = 0; i < bmp.GetRawDataSize(); ++i )
	{
		pixels[i] = rand() & 0xff;
	}
	memcpy( bmp.GetRawData(), pixels.get(), bmp.GetRawDataSize() );

	EXPECT_TRUE( bmp.ConvertFormat( PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, bmp.GetFormat() );
	for( size_t i = 0; i < bmp.GetRawDataSize(); ++i )
	{
		if( i % 4 == 3 )
		{
			EXPECT_EQ( 0xff, reinterpret_cast<uint8_t*>( bmp.GetRawData() )[i] );
		}
		else
		{
			EXPECT_EQ( pixels[i / 4], reinterpret_cast<uint8_t*>( bmp.GetRawData() )[i] );
		}
	}
}

TEST( HostBitmap, CanConvertL8A8ToB8G8R8A8 )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 8, 8, 0, PIXEL_FORMAT_R8G8_UNORM ) );
	std::unique_ptr<uint8_t[]> pixels( new uint8_t[bmp.GetRawDataSize()] );
	for( size_t i = 0; i < bmp.GetRawDataSize(); ++i )
	{
		pixels[i] = rand() & 0xff;
	}
	memcpy( bmp.GetRawData(), pixels.get(), bmp.GetRawDataSize() );

	EXPECT_TRUE( bmp.ConvertFormat( PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	EXPECT_EQ( PIXEL_FORMAT_B8G8R8A8_UNORM, bmp.GetFormat() );
	for( size_t i = 0; i < bmp.GetRawDataSize(); ++i )
	{
		if( i % 4 == 3 )
		{
			EXPECT_EQ( pixels[i / 4 * 2 + 1], reinterpret_cast<uint8_t*>( bmp.GetRawData() )[i] );
		}
		else
		{
			EXPECT_EQ( pixels[i / 4 * 2], reinterpret_cast<uint8_t*>( bmp.GetRawData() )[i] );
		}
	}
}

TEST( HostBitmap, GenerateMipmapsFailsForNon8BitsPerChannelImages )
{
	PixelFormat formats[] = {
		PIXEL_FORMAT_UNKNOWN,
		PIXEL_FORMAT_R32G32B32A32_TYPELESS,
		PIXEL_FORMAT_R32G32B32A32_FLOAT,
		PIXEL_FORMAT_R32G32B32A32_UINT,
		PIXEL_FORMAT_R32G32B32A32_SINT,
		PIXEL_FORMAT_R32G32B32_TYPELESS,
		PIXEL_FORMAT_R32G32B32_FLOAT,
		PIXEL_FORMAT_R32G32B32_UINT,
		PIXEL_FORMAT_R32G32B32_SINT,
		PIXEL_FORMAT_R16G16B16A16_TYPELESS,
		PIXEL_FORMAT_R16G16B16A16_FLOAT,
		PIXEL_FORMAT_R16G16B16A16_UNORM,
		PIXEL_FORMAT_R16G16B16A16_UINT,
		PIXEL_FORMAT_R16G16B16A16_SNORM,
		PIXEL_FORMAT_R16G16B16A16_SINT,
		PIXEL_FORMAT_R32G32_TYPELESS,
		PIXEL_FORMAT_R32G32_FLOAT,
		PIXEL_FORMAT_R32G32_UINT,
		PIXEL_FORMAT_R32G32_SINT,
		PIXEL_FORMAT_R32G8X24_TYPELESS,
		PIXEL_FORMAT_D32_FLOAT_S8X24_UINT,
		PIXEL_FORMAT_R32_FLOAT_X8X24_TYPELESS,
		PIXEL_FORMAT_X32_TYPELESS_G8X24_UINT,
		PIXEL_FORMAT_R10G10B10A2_TYPELESS,
		PIXEL_FORMAT_R10G10B10A2_UNORM,
		PIXEL_FORMAT_R10G10B10A2_UINT,
		PIXEL_FORMAT_R11G11B10_FLOAT,
		PIXEL_FORMAT_R16G16_TYPELESS,
		PIXEL_FORMAT_R16G16_FLOAT,
		PIXEL_FORMAT_R16G16_UNORM,
		PIXEL_FORMAT_R16G16_UINT,
		PIXEL_FORMAT_R16G16_SNORM,
		PIXEL_FORMAT_R16G16_SINT,
		PIXEL_FORMAT_R32_TYPELESS,
		PIXEL_FORMAT_D32_FLOAT,
		PIXEL_FORMAT_R32_FLOAT,
		PIXEL_FORMAT_R32_UINT,
		PIXEL_FORMAT_R32_SINT,
		PIXEL_FORMAT_R24G8_TYPELESS,
		PIXEL_FORMAT_D24_UNORM_S8_UINT,
		PIXEL_FORMAT_R24_UNORM_X8_TYPELESS,
		PIXEL_FORMAT_X24_TYPELESS_G8_UINT,
		PIXEL_FORMAT_R16_TYPELESS,
		PIXEL_FORMAT_R16_FLOAT,
		PIXEL_FORMAT_D16_UNORM,
		PIXEL_FORMAT_R16_UNORM,
		PIXEL_FORMAT_R16_UINT,
		PIXEL_FORMAT_R16_SNORM,
		PIXEL_FORMAT_R16_SINT,
		PIXEL_FORMAT_R9G9B9E5_SHAREDEXP,
		PIXEL_FORMAT_BC1_TYPELESS,
		PIXEL_FORMAT_BC1_UNORM,
		PIXEL_FORMAT_BC1_UNORM_SRGB,
		PIXEL_FORMAT_BC2_TYPELESS,
		PIXEL_FORMAT_BC2_UNORM,
		PIXEL_FORMAT_BC2_UNORM_SRGB,
		PIXEL_FORMAT_BC3_TYPELESS,
		PIXEL_FORMAT_BC3_UNORM,
		PIXEL_FORMAT_BC3_UNORM_SRGB,
		PIXEL_FORMAT_BC4_TYPELESS,
		PIXEL_FORMAT_BC4_UNORM,
		PIXEL_FORMAT_BC4_SNORM,
		PIXEL_FORMAT_BC5_TYPELESS,
		PIXEL_FORMAT_BC5_UNORM,
		PIXEL_FORMAT_BC5_SNORM,
		PIXEL_FORMAT_B5G6R5_UNORM,
		PIXEL_FORMAT_B5G5R5A1_UNORM,
		PIXEL_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
		PIXEL_FORMAT_BC6H_TYPELESS,
		PIXEL_FORMAT_BC6H_UF16,
		PIXEL_FORMAT_BC6H_SF16,
		PIXEL_FORMAT_BC7_TYPELESS,
		PIXEL_FORMAT_BC7_UNORM,
		PIXEL_FORMAT_BC7_UNORM_SRGB,
	};
	for( int i = 0; i < sizeof( formats ) / sizeof( PixelFormat ); ++i )
	{
		HostBitmap bmp;
		ASSERT_TRUE( bmp.CreateVolume( 8, 8, 8, 1, formats[i] ) );
		EXPECT_FALSE( bmp.GenerateMipMaps() );
	}
}

TEST( HostBitmap, CanGenerateMipmaps )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 5, 3, 1, PIXEL_FORMAT_A8_UNORM ) );
	uint8_t pixels[] = {
		1, 3, 5, 5, 3, 
		6, 4, 4, 2, 7,
		9, 3, 8, 4, 0,
	};
	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );

	EXPECT_TRUE( bmp.GenerateMipMaps() );
	EXPECT_EQ( 3, bmp.GetMipCount() );
	EXPECT_EQ( 3, bmp.GetTrueMipCount() );

	uint8_t expectedPixels1[] = {
		3, 4,
	};
	EXPECT_EQ( 0, memcmp( expectedPixels1, bmp.GetMipRawData( 1 ), sizeof( expectedPixels1 ) ) );

	uint8_t expectedPixels2[] = {
		3,
	};
	EXPECT_EQ( 0, memcmp( expectedPixels2, bmp.GetMipRawData( 2 ), sizeof( expectedPixels2 ) ) );
}

TEST( HostBitmap, CanGenerateSpecificNumberOfMipmaps )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 5, 3, 1, PIXEL_FORMAT_A8_UNORM ) );
	EXPECT_TRUE( bmp.GenerateMipMaps( 2 ) );
	EXPECT_EQ( 2, bmp.GetMipCount() );
}

TEST( HostBitmap, CanDropMipmaps )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 5, 3, 1, PIXEL_FORMAT_A8_UNORM ) );
	uint8_t pixels[15] = {};
	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );

	EXPECT_TRUE( bmp.GenerateMipMaps() );
	EXPECT_EQ( 3, bmp.GetMipCount() );

	EXPECT_TRUE( bmp.DropMipMaps() );
	EXPECT_EQ( 1, bmp.GetMipCount() );
}

TEST( HostBitmap, CanCopyChannel )
{
	HostBitmap bmp1, bmp2;
	ASSERT_TRUE( bmp1.Create( 5, 3, 1, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	ASSERT_TRUE( bmp2.Create( 5, 3, 1, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	
	EXPECT_TRUE( bmp1.CopyChannel( &bmp2, 3, 3 ) );
	EXPECT_TRUE( bmp1.CopyChannel( &bmp1, 0, 1 ) );
}

TEST( HostBitmap, CannotCopyChannelBetweenIncompatibleBitmaps )
{
	HostBitmap bmp1, bmp2;

	ASSERT_TRUE( bmp1.Create2DArray( 5, 3, 1, 3, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	ASSERT_TRUE( bmp2.Create2DArray( 5, 3, 1, 2, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	EXPECT_FALSE( bmp1.CopyChannel( &bmp2, 3, 3 ) );

	ASSERT_TRUE( bmp1.Create( 5, 3, 1, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	ASSERT_TRUE( bmp2.Create2DArray( 5, 3, 1, 2, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	EXPECT_FALSE( bmp1.CopyChannel( &bmp2, 3, 3 ) );
	
	ASSERT_TRUE( bmp1.Create( 5, 4, 1, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	ASSERT_TRUE( bmp2.Create( 5, 3, 1, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	EXPECT_FALSE( bmp1.CopyChannel( &bmp2, 3, 3 ) );
	
	ASSERT_TRUE( bmp1.Create( 5, 3, 1, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	ASSERT_TRUE( bmp2.Create( 4, 3, 1, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	EXPECT_FALSE( bmp1.CopyChannel( &bmp2, 3, 3 ) );
	
	ASSERT_TRUE( bmp1.Create( 5, 3, 2, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	ASSERT_TRUE( bmp2.Create( 5, 3, 1, PIXEL_FORMAT_B8G8R8A8_UNORM) );
	EXPECT_FALSE( bmp1.CopyChannel( &bmp2, 3, 3 ) );
}

TEST( HostBitmap, CanRotateBitmap )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 3, 3, 1, PIXEL_FORMAT_A8_UNORM ) );
	uint8_t pixels[] = {
		1, 2, 3,
		4, 5, 6,
		7, 8, 9
	};
	uint8_t pixelsRotated90[] = {
		7, 4, 1,
		8, 5, 2,
		9, 6, 3
	};
	uint8_t pixelsRotated180[] = {
		9, 8, 7,
		6, 5, 4,
		3, 2, 1
	};
	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );

	EXPECT_TRUE( bmp.RotateFaceClockwise( 0, 1 ) );

	for( unsigned i = 0; i < 9; i++ )
	{
		EXPECT_EQ( bmp.GetRawData()[i], pixelsRotated90[i] );
	}

	EXPECT_TRUE( bmp.RotateFaceClockwise( 0, 5 ) );

	for( unsigned i = 0; i < 9; i++ )
	{
		EXPECT_EQ( bmp.GetRawData()[i], pixelsRotated180[i] );
	}

	ASSERT_TRUE( bmp.Create2DArray( 3, 3, 1, 2, PIXEL_FORMAT_A8_UNORM ) );
	uint8_t pixels2D[] = {
		1, 2, 3,
		4, 5, 6,
		7, 8, 9,

		10,11,12, 
		13,14,15,
		16,17,18,
	};
	uint8_t pixels2DRotated270[] = {
		1, 2, 3,
		4, 5, 6,
		7, 8, 9,

		12,15,18, 
		11,14,17,
		10,13,16,
	};
	memcpy( bmp.GetRawData(), pixels2D, sizeof( pixels2D ) );

	EXPECT_TRUE( bmp.RotateFaceClockwise( 1, 3 ) );

	for( unsigned i = 0; i < 18; i++ )
	{
		EXPECT_EQ( bmp.GetRawData()[i], pixels2DRotated270[i] );
	}
}

TEST( HostBitmap, BitmapSizeIsSensible )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 1920, 1080, 0, PIXEL_FORMAT_BC1_UNORM ) );
	size_t size = 0;
	for( uint32_t i = 0; i < bmp.GetTrueMipCount(); ++i )
	{
		size += bmp.GetMipSize( i );
	}
	EXPECT_EQ( size, bmp.GetRawDataSize() );
}


TEST( HostBitmap, GetPixelReturnsCorrectValueForBGRXBitmaps )
{
	HostBitmap bitmap;
	ASSERT_TRUE( LoadBitmap( s_rgbx, bitmap ) );
	float r, g, b, a;
	ASSERT_TRUE( bitmap.GetPixel( 0, 3, r, g, b, a ) );
	EXPECT_EQ( 250, int( r * 255 ) );
	EXPECT_EQ( 1, int( g * 255 ) );
	EXPECT_EQ( 242, int( b * 255 ) );
	EXPECT_EQ( 255, int( a * 255 ) );
}


TEST( HostBitmap, GetPixelReturnsCorrectValueForBGRABitmaps )
{
	HostBitmap bitmap;
	ASSERT_TRUE( LoadBitmap( s_rgba, bitmap ) );
	float r, g, b, a;
	ASSERT_TRUE( bitmap.GetPixel( 4, 8, r, g, b, a ) );
	EXPECT_EQ( 108, int( r * 255 ) );
	EXPECT_EQ( 141, int( g * 255 ) );
	EXPECT_EQ( 245, int( b * 255 ) );
	EXPECT_EQ( 22, int( a * 255 ) );
}

TEST( HostBitmap, GetPixelReturnsCorrectValueForBC1Bitmaps )
{
	HostBitmap bitmap;
	ASSERT_TRUE( LoadBitmap( s_bc1, bitmap ) );
	float r, g, b, a;
	ASSERT_TRUE( bitmap.GetPixel( 1, 0, r, g, b, a ) );
	EXPECT_EQ( 123, int( r * 255 ) );
	EXPECT_EQ( 125, int( g * 255 ) );
	EXPECT_EQ( 123, int( b * 255 ) );
	EXPECT_EQ( 255, int( a * 255 ) );
}

TEST( HostBitmap, GetPixelReturnsCorrectValueForBC3Bitmaps )
{
	HostBitmap bitmap;
	ASSERT_TRUE( LoadBitmap( s_bc3, bitmap ) );
	float r, g, b, a;
	ASSERT_TRUE( bitmap.GetPixel( 0, 0, r, g, b, a ) );
	EXPECT_EQ( 1.0f, r );
	EXPECT_EQ( 1.0f, g );
	EXPECT_EQ( 1.0f, b );
	EXPECT_EQ( 0.0f, a );
}

TEST( HostBitmap, GetAverageColorOnBlackTextureWorks )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 4, 2, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	uint8_t pixels[] = {
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	};
	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );

	float r, g, b, a;
	ASSERT_TRUE( bmp.GetAverageColor( r, g, b, a ) );
	EXPECT_EQ( 0, r );
	EXPECT_EQ( 0, g );
	EXPECT_EQ( 0, b );
	EXPECT_EQ( 0, a );
}

TEST( HostBitmap, GetAverageColorGetsAverageOfGrid)
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 4, 2, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	uint8_t pixels[] = {
		0, 0, 0, 0,  255, 0, 0, 0,  0, 0, 0, 0,  0, 0, 255, 0,
		0, 0, 0, 0,  0, 255, 0, 0,  0, 0, 0, 0,  0, 0, 0, 255,
	};
	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );
	float r, g, b, a;
	ASSERT_TRUE( bmp.GetAverageColor( r, g, b, a ) );
	EXPECT_EQ( 1.0f / 4.f, r );
	EXPECT_EQ( 1.0f / 4.f, g );
	EXPECT_EQ( 1.0f / 4.f, b );
	EXPECT_EQ( 1.0f / 4.f, a );
}

TEST( HostBitmap, GetAverageColorSkipsEveryOtherPixelWhenImagesAreLargerThan128Pixels )
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 16, 16, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	uint8_t pixels[256 * 4] = { };

	for( uint32_t i = 0; i < 256; ++i ) {
		uint32_t m = (i + 1) % 2;
		pixels[i * 4] = 255 * m;
		pixels[i * 4 + 1] = 255 * m;
		pixels[i * 4 + 2] = 255 * m;
		pixels[i * 4 + 3] = 255 * m;
	}

	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );
	float r, g, b, a;
	ASSERT_TRUE( bmp.GetAverageColor( r, g, b, a ) );
	EXPECT_EQ( 255, r * 255 );
	EXPECT_EQ( 255, g * 255 );
	EXPECT_EQ( 255, b * 255 );
	EXPECT_EQ( 255, a * 255 );
}

TEST( HostBitmap, GetAverageColorTakesTheAverageColorOfTheImage)
{
	HostBitmap bmp;
	ASSERT_TRUE( bmp.Create( 2, 2, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ) );
	uint8_t pixelChoice[4 * 4] = {
		123, 60, 1, 0,
		255, 0, 0, 255,
		0, 255, 0, 0,
		255, 0, 255, 0,
	};
	uint8_t pixels[4 * 4] = {};

	for( uint32_t i = 0; i < 4; ++i ) {
		
		uint32_t pixelIndex = (i % 4) * 4;

		pixels[i * 4] = pixelChoice[pixelIndex];
		pixels[i * 4 + 1] = pixelChoice[pixelIndex + 1];
		pixels[i * 4 + 2] = pixelChoice[pixelIndex + 2];
		pixels[i * 4 + 3] = pixelChoice[pixelIndex + 3];
	}

	memcpy( bmp.GetRawData(), pixels, sizeof( pixels ) );
	float r, g, b, a;
	ASSERT_TRUE( bmp.GetAverageColor( r, g, b, a ) );
	EXPECT_EQ( (123 + 255 + 0 + 255) / 4, int( b * 255 ));
	EXPECT_EQ( (60 + 0 + 255 + 0) / 4, int( g * 255 ));
	EXPECT_EQ( (1 + 0 + 0 + 255) / 4, int( r * 255 ));
	EXPECT_EQ( (0 + 255 + 0 + 0) / 4, int( a * 255 ));
}

INSTANTIATE_TEST_SUITE_P( HostBitmap,
                        PixelFormatTest,
						::testing::Range( int( PIXEL_FORMAT_UNKNOWN + 1 ), int( PIXEL_FORMAT_SENTINEL ) ) );
