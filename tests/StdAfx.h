#define NOMINMAX

#include "CcpCore/include/CcpCore.h"
#include "TrinityAL/include/Tr2PixelFormat.h"
#include "TrinityAL/include/Tr2TextureType.h"
#include "TrinityAL/include/Tr2BitmapDimensions.h"
#include "TrinityAL/include/Tr2CubemapFace.h"

#include <csetjmp>
#include <cmath>
#include <memory>
#include <limits>

#include "gtest/gtest.h"

#include "ImageIO/HostBitmap.h"
#include "ImageIO/Tr2ImageHandler.h"
#include "ImageIO/Tr2BmpHandler.h"
#include "ImageIO/Tr2DdsHandler.h"
#include "ImageIO/Tr2JpgHandler.h"
#include "ImageIO/Tr2PngHandler.h"
#include "ImageIO/Tr2TgaHandler.h"
#include "ImageIO/PsdHandler.h"
#include "ImageIO/ImageUtility.h"
