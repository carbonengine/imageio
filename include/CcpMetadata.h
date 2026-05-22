// Copyright © 2020 CCP ehf.

#pragma once

#include "Tr2ImageHandler.h"

namespace ImageIO
{
	Result LoadCcpMetadata( ICcpStream& stream, Metadata& metadata );
	Result SaveCcpMetadata( ICcpStream& stream, const Metadata& metadata );
}