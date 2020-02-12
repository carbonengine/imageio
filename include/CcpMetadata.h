////////////////////////////////////////////////////////////
//
//    Creator:   Filipp Pavlov
//    Created:   February 2020
//    Copyright: CCP 2020
//

#pragma once

#include "Tr2ImageHandler.h"

namespace ImageIO
{
	Result LoadCcpMetadata( ICcpStream& stream, Metadata& metadata );
	Result SaveCcpMetadata( ICcpStream& stream, const Metadata& metadata );
}