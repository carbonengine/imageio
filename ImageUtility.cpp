// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "ImageUtility.h"
#include "include/PixelFormat.h"

namespace ImageUtility {

	inline uint32_t InterpolatedColor( uint32_t color0, uint32_t weight0, uint32_t color1, uint32_t weight1, uint32_t offset, uint32_t divisor )
	{
		uint32_t r0 = (color0 >> 16) & 0xff;
		uint32_t g0 = (color0 >> 8) & 0xff;
		uint32_t b0 = (color0 >> 0) & 0xff;
		uint32_t r1 = (color1 >> 16) & 0xff;
		uint32_t g1 = (color1 >> 8) & 0xff;
		uint32_t b1 = (color1 >> 0) & 0xff;

		uint32_t r = (r0 * weight0 + r1 * weight1 + offset) / divisor & 0xff;
		uint32_t g = (g0 * weight0 + g1 * weight1 + offset) / divisor & 0xff;
		uint32_t b = (b0 * weight0 + b1 * weight1 + offset) / divisor & 0xff;
		return (r << 16) | (g << 8) | b;
	}

	uint32_t ConvertBGR565A8ToBGRA8( uint32_t color, uint32_t alpha )
	{
		return uint32_t( color & 0x1f ) * 255 / 31 |
			( uint32_t( ( color >> 5 ) & 0x3f ) * 255 / 63 ) << 8 |
			( uint32_t( ( color >> 11 ) & 0x1f ) * 255 / 31 ) << 16 |
			( alpha << 24 );
	}

	uint32_t GetPixelColor_BGRA( uint32_t x, uint32_t y, uint32_t pitch, const char* source )
	{
		return reinterpret_cast<const uint32_t*>( source + y * pitch )[x];
	}

	uint32_t GetPixelColor_R( uint32_t x, uint32_t y, uint32_t pitch, const char* source )
	{
		return uint32_t( reinterpret_cast<const uint8_t*>( source + y * pitch )[x] ) << 16;
	}

	uint32_t GetPixelColor_BGRX( uint32_t x, uint32_t y, uint32_t pitch, const char* source )
	{
		return GetPixelColor_BGRA( x, y, pitch, source ) | 0xff000000;
	}

	uint32_t GetPixelColor_BC1( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* source )
	{
		// get the block index. ((width + 3) / 4) is basically a ceiling operator!
		unsigned index = x / 4 + (y / 4) * ((width + 3) / 4);
		index *= GetBlockByteSize( ImageIO::PIXEL_FORMAT_BC1_UNORM );
		uint32_t pixelValue;
		uint32_t color0 = *reinterpret_cast<const uint16_t*>( source + index );
		uint32_t color1 = *reinterpret_cast<const uint16_t*>( source + index + 2 );
		uint32_t bits = *reinterpret_cast<const uint32_t*>( source + index + 4 );

		x = x % 4;
		y = y % 4;

		if( color0 > color1 )
		{
			switch( (bits >> 2 * (4 * y + x)) & 3 )
			{
			case 0:
				pixelValue = ConvertBGR565A8ToBGRA8( color0, 255 );
				break;
			case 1:
				pixelValue = ConvertBGR565A8ToBGRA8( color1, 255 );
				break;
			case 2:
				pixelValue = ConvertBGR565A8ToBGRA8( (2 * color0 + color1) / 3, 255 );
				break;
			default:
				pixelValue = ConvertBGR565A8ToBGRA8( (color0 + 2 * color1) / 3, 255 );
				break;
			}
		}
		else
		{
			switch( (bits >> 2 * (4 * y + x)) & 3 )
			{
			case 0:
				pixelValue = ConvertBGR565A8ToBGRA8( color0, 255 );
				break;
			case 1:
				pixelValue = ConvertBGR565A8ToBGRA8( color1, 255 );
				break;
			case 2:
				pixelValue = ConvertBGR565A8ToBGRA8( (color0 + color1) / 2, 255 );
				break;
			default:
				pixelValue = ConvertBGR565A8ToBGRA8( (color0 + 2 * color1) / 3, 0 );
				break;
			}
		}
		return pixelValue;
	}
	
	uint32_t GetPixelColor_BC3( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* source )
	{
		unsigned index = x / 4 + (y / 4) * ((width + 3) / 4);
		index *= GetBlockByteSize( ImageIO::PIXEL_FORMAT_BC3_UNORM );
		unsigned pixelValue;
		
		uint32_t alpha[8];
		alpha[0] = *reinterpret_cast<const uint8_t*>(source);
		alpha[1] = *reinterpret_cast<const uint8_t*>(source + 1);
		auto alphaMask = source + 2;
		uint32_t alphaMask0 = (alphaMask[0]) | (alphaMask[1] << 8) | (alphaMask[2] << 16);
		uint32_t alphaMask1 = (alphaMask[3]) | (alphaMask[4] << 8) | (alphaMask[5] << 16);
		if( alpha[0] > alpha[1] )
		{
			alpha[2] = (6 * alpha[0] + 1 * alpha[1] + 3) / 7;
			alpha[3] = (5 * alpha[0] + 2 * alpha[1] + 3) / 7;
			alpha[4] = (4 * alpha[0] + 3 * alpha[1] + 3) / 7;
			alpha[5] = (3 * alpha[0] + 4 * alpha[1] + 3) / 7;
			alpha[6] = (2 * alpha[0] + 5 * alpha[1] + 3) / 7;
			alpha[7] = (1 * alpha[0] + 6 * alpha[1] + 3) / 7;
		}
		else
		{
			alpha[2] = (4 * alpha[0] + 1 * alpha[1] + 2) / 5;
			alpha[3] = (3 * alpha[0] + 2 * alpha[1] + 2) / 5;
			alpha[4] = (2 * alpha[0] + 3 * alpha[1] + 2) / 5;
			alpha[5] = (1 * alpha[0] + 4 * alpha[1] + 2) / 5;
			alpha[6] = 0;
			alpha[7] = 255;
		}

		uint32_t color0 = ConvertBGR565A8ToBGRA8( *reinterpret_cast<const uint16_t*>(source + 8), 0 );
		uint32_t color1 = ConvertBGR565A8ToBGRA8( *reinterpret_cast<const uint16_t*>(source + 8 + 2), 0 );
		uint32_t color2 = InterpolatedColor( color0, 2, color1, 1, 1, 3 );
		uint32_t color3 = InterpolatedColor( color0, 1, color1, 2, 1, 3 );
		uint32_t bits = *reinterpret_cast<const uint32_t*>(source + 8 + 4);

		x = x % 4;
		y = y % 4;

		uint32_t alphaValue;
		if( y < 2 )
		{
			alphaValue = alpha[(alphaMask0 >> (x + y * 4) * 3) & 0x7];
		}
		else
		{
			alphaValue = alpha[(alphaMask1 >> (x + (y - 2) * 4) * 3) & 0x7];
		}
		alphaValue <<= 24;
		switch( (bits >> 2 * (4 * y + x)) & 3 )
		{
		case 0:
			pixelValue = color0 | alphaValue;
			break;
		case 1:
			pixelValue = color1 | alphaValue;
			break;
		case 2:
			pixelValue = color2 | alphaValue;
			break;
		default:
			pixelValue = color3 | alphaValue;
			break;
		}
		return pixelValue;
	}

}
