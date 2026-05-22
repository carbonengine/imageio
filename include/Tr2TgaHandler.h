// Copyright © 2012 CCP ehf.

#pragma once
#ifndef Tr2TgaHandler_h
#define Tr2TgaHandler_h

#include "Tr2ImageHandler.h"

namespace ImageIO
{
namespace Tga
{

void RegisterHandler();
bool IsTgaExtension( const wchar_t* extension );
Result ReadImage( ICcpStream& src, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata );
Result IsSaveSupported( const BitmapDimensions& bd );
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata );

Result IsSaveSupported( PixelFormat format );
Result SaveHeader( uint32_t width, uint32_t height, PixelFormat format, ICcpStream& output );
Result SaveRows( uint32_t width, uint32_t height, PixelFormat format, const void* data, ICcpStream& output );

}
}

#endif//Tr2TgaHandler_h_h