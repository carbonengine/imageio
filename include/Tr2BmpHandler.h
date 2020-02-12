////////////////////////////////////////////////////////////
//
//    Creator:   Filipp Pavlov
//    Created:   July 2012
//    Copyright: CCP 2012
//

#pragma once
#ifndef Tr2BmpHandler_h
#define Tr2BmpHandler_h

#include "Tr2ImageHandler.h"

namespace ImageIO
{
namespace Bmp
{

void RegisterHandler();
bool IsBmpExtension( const wchar_t* extension );
Result ReadImage( ICcpStream& src, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata );
Result IsSaveSupported( const Tr2BitmapDimensions& bd );
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata );

}
}

#endif//Tr2BmpHandler_h_h