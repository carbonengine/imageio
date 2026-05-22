// Copyright © 2014 CCP ehf.

#pragma once
#ifndef PsdHandler_H
#define PsdHandler_H

#include "Tr2ImageHandler.h"

namespace ImageIO
{

namespace Psd
{

void RegisterHandler();
bool IsPsdExtension( const wchar_t* extension );
Result ReadImage( ICcpStream& src, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata );
Result IsSaveSupported( const BitmapDimensions& bd );
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata );

}

}

#endif