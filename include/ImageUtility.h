// Copyright © 2018 CCP ehf.

#pragma once

#ifndef ImageUtility_H
#define ImageUtility_H

namespace ImageUtility {
	uint32_t GetPixelColor_R( uint32_t x, uint32_t y, uint32_t pitch, const char* source );
	uint32_t GetPixelColor_BGRA( uint32_t x, uint32_t y, uint32_t pitch, const char* source );
	uint32_t GetPixelColor_BGRX( uint32_t x, uint32_t y, uint32_t pitch, const char* source );
	uint32_t GetPixelColor_BC1( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* source );
	uint32_t GetPixelColor_BC3( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* source );
}

#endif