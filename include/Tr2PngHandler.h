#pragma once

#ifndef Tr2PngHandler_h
#define Tr2PngHandler_h

#include "Tr2ImageHandler.h"

namespace ImageIO
{
namespace Png
{

void RegisterHandler();
bool IsPngExtension( const wchar_t* extension );
Result ReadImage( ICcpStream& src, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata );
Result IsSaveSupported( const Tr2BitmapDimensions& bd );
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata );

}
}

#endif // Tr2PngHandler_h
