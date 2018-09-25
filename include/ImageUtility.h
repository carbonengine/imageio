#pragma once

#ifndef ImageUtility_H
#define ImageUtility_H

namespace ImageUtility {
	unsigned GetPixelColor_BGRA( uint32_t x, uint32_t y, uint32_t pitch, const char* source );
	unsigned GetPixelColor_BGRX( uint32_t x, uint32_t y, uint32_t pitch, const char* source );
	unsigned GetPixelColor_BC1( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* source );
	unsigned GetPixelColor_BC3( uint32_t x, uint32_t y, uint32_t width, uint32_t pitch, const char* source );
}

#endif