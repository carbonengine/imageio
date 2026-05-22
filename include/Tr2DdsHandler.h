// Copyright © 2014 CCP ehf.

#pragma once

#ifndef Tr2DdsHandler_h_
#define Tr2DdsHandler_h_

#include "Tr2ImageHandler.h"

namespace ImageIO
{
namespace Dds
{

void RegisterHandler();
bool IsDdsExtension( const wchar_t* extension );
Result ReadImage( ICcpStream& src, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata );
Result IsSaveSupported( const BitmapDimensions& bd );
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata );

}
}

#endif
