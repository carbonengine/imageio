// Copyright © 2014 CCP ehf.

#pragma once
#ifndef Tr2JpgHandler_h
#define Tr2JpgHandler_h

#include "Tr2ImageHandler.h"

namespace ImageIO
{
namespace Jpeg
{

void RegisterHandler();
bool IsJpegExtension( const wchar_t* extension );
Result ReadImage( ICcpStream& src, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata );
Result IsSaveSupported( const BitmapDimensions& bd );
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata );

}
}

#endif//Tr2JpgHandler_h