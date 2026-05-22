// Copyright © 2014 CCP ehf.

#include "StdAfx.h"

#include "Tr2DdsHandler.h"
#include "HostBitmap.h"
#include "CcpMetadata.h"

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
	((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
	((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

#define DDS_FOURCC	0x00000004 // DDPF_FOURCC
#define DDS_INDEXED 0x00000020 // DDPF_INDEXED
#define DDS_RGB     0x00000040 // DDPF_RGB
#define DDS_RGBA    0x00000041 // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCE 0x20000

#define DDS_HEADER_FLAGS_TEXTURE    0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
#define DDS_HEADER_FLAGS_MIPMAP     0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME     0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH      0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE 0x00080000  // DDSD_LINEARSIZE

#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
	DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
	DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define DDS_FLAGS_VOLUME 0x00200000 // DDSCAPS2_VOLUME

#define D3D10_RESOURCE_DIMENSION_UNKNOWN   0
#define D3D10_RESOURCE_DIMENSION_BUFFER    1
#define D3D10_RESOURCE_DIMENSION_TEXTURE1D 2
#define D3D10_RESOURCE_DIMENSION_TEXTURE2D 3
#define D3D10_RESOURCE_DIMENSION_TEXTURE3D 4

using namespace ImageIO;

namespace
{

const unsigned int FOURCC_DDS = MAKEFOURCC('D', 'D', 'S', ' ');
const unsigned int FOURCC_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
const unsigned int FOURCC_DXT2 = MAKEFOURCC('D', 'X', 'T', '2');
const unsigned int FOURCC_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
const unsigned int FOURCC_DXT4 = MAKEFOURCC('D', 'X', 'T', '4');
const unsigned int FOURCC_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');
const unsigned int FOURCC_DX10 = MAKEFOURCC('D', 'X', '1', '0');
const unsigned int FOURCC_RXGB = MAKEFOURCC('R', 'X', 'G', 'B');
const unsigned int FOURCC_ATI1 = MAKEFOURCC('A', 'T', 'I', '1');
const unsigned int FOURCC_ATI2 = MAKEFOURCC('A', 'T', 'I', '2');

struct DDS_PIXELFORMAT
{
	uint32_t dwSize;			// 4
	uint32_t dwFlags;
	uint32_t dwFourCC;
	uint32_t dwRGBBitCount;	// 16
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;		// 32
};

struct DDS_HEADER
{
	uint32_t dwFourCC;				// 4
	uint32_t dwSize;
	uint32_t dwHeaderFlags;
	uint32_t dwHeight;				// 16
	uint32_t dwWidth;
	uint32_t dwPitchOrLinearSize;
	uint32_t dwDepth;				// only if DDS_HEADER_FLAGS_VOLUME is set in dwHeaderFlags
	uint32_t dwMipMapCount;		// 32
	uint32_t dwReserved1[11];		// 76
	DDS_PIXELFORMAT ddspf;		// 108
	uint32_t dwSurfaceFlags;
	uint32_t dwCubemapFlags;		// 116
	uint32_t dwReserved2[3];		// 128
};

struct DDS_HEADER_DXT10
{
	PixelFormat dxgiFormat;
	uint32_t resourceDimension;
	uint32_t miscFlag;
	uint32_t arraySize;
	uint32_t miscFlags2;
};

struct FormatDescriptor
{
	int32_t format;
	unsigned int bitcount;
	unsigned int rmask;
	unsigned int gmask;
	unsigned int bmask;
	unsigned int amask;
	unsigned int fourCC;

	PixelFormat	pixelFormat;
};

#define DDS_MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
            ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |			\
            ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))


const int32_t DDSFMT_UNKNOWN              =  0;
const int32_t DDSFMT_R8G8B8               = 20;
const int32_t DDSFMT_A8R8G8B8             = 21;
const int32_t DDSFMT_X8R8G8B8             = 22;
const int32_t DDSFMT_R5G6B5               = 23;
const int32_t DDSFMT_X1R5G5B5             = 24;
const int32_t DDSFMT_A1R5G5B5             = 25;
const int32_t DDSFMT_A4R4G4B4             = 26;
const int32_t DDSFMT_R3G3B2               = 27;
const int32_t DDSFMT_A8                   = 28;
const int32_t DDSFMT_A8R3G3B2             = 29;
const int32_t DDSFMT_X4R4G4B4             = 30;
const int32_t DDSFMT_A2B10G10R10          = 31;
const int32_t DDSFMT_A8B8G8R8             = 32;
const int32_t DDSFMT_X8B8G8R8             = 33;
const int32_t DDSFMT_G16R16               = 34;
const int32_t DDSFMT_A2R10G10B10          = 35;
const int32_t DDSFMT_A16B16G16R16         = 36;
const int32_t DDSFMT_A8P8                 = 40;
const int32_t DDSFMT_P8                   = 41;
const int32_t DDSFMT_L8                   = 50;
const int32_t DDSFMT_A8L8                 = 51;
const int32_t DDSFMT_A4L4                 = 52;
const int32_t DDSFMT_V8U8                 = 60;
const int32_t DDSFMT_L6V5U5               = 61;
const int32_t DDSFMT_X8L8V8U8             = 62;
const int32_t DDSFMT_Q8W8V8U8             = 63;
const int32_t DDSFMT_V16U16               = 64;
const int32_t DDSFMT_A2W10V10U10          = 67;
const int32_t DDSFMT_UYVY                 = DDS_MAKEFOURCC('U', 'Y', 'V', 'Y');
const int32_t DDSFMT_R8G8_B8G8            = DDS_MAKEFOURCC('R', 'G', 'B', 'G');
const int32_t DDSFMT_YUY2                 = DDS_MAKEFOURCC('Y', 'U', 'Y', '2');
const int32_t DDSFMT_G8R8_G8B8            = DDS_MAKEFOURCC('G', 'R', 'G', 'B');
const int32_t DDSFMT_DXT1                 = DDS_MAKEFOURCC('D', 'X', 'T', '1');
const int32_t DDSFMT_DXT2                 = DDS_MAKEFOURCC('D', 'X', 'T', '2');
const int32_t DDSFMT_DXT3                 = DDS_MAKEFOURCC('D', 'X', 'T', '3');
const int32_t DDSFMT_DXT4                 = DDS_MAKEFOURCC('D', 'X', 'T', '4');
const int32_t DDSFMT_DXT5                 = DDS_MAKEFOURCC('D', 'X', 'T', '5');
const int32_t DDSFMT_DX10                 = DDS_MAKEFOURCC('D', 'X', '1', '0');
const int32_t DDSFMT_D16_LOCKABLE         = 70;
const int32_t DDSFMT_D32                  = 71;
const int32_t DDSFMT_D15S1                = 73;
const int32_t DDSFMT_D24S8                = 75;
const int32_t DDSFMT_D24X8                = 77;
const int32_t DDSFMT_D24X4S4              = 79;
const int32_t DDSFMT_D16                  = 80;
const int32_t DDSFMT_D32F_LOCKABLE        = 82;
const int32_t DDSFMT_D24FS8               = 83;
const int32_t DDSFMT_D32_LOCKABLE         = 84;
const int32_t DDSFMT_S8_LOCKABLE          = 85;
const int32_t DDSFMT_L16                  = 81;
const int32_t DDSFMT_VERTEXDATA           =100;
const int32_t DDSFMT_INDEX16              =101;
const int32_t DDSFMT_INDEX32              =102;
const int32_t DDSFMT_Q16W16V16U16         =110;
const int32_t DDSFMT_MULTI2_ARGB8         = DDS_MAKEFOURCC('M','E','T','1');
const int32_t DDSFMT_R16F                 = 111;
const int32_t DDSFMT_G16R16F              = 112;
const int32_t DDSFMT_A16B16G16R16F        = 113;
const int32_t DDSFMT_R32F                 = 114;
const int32_t DDSFMT_G32R32F              = 115;
const int32_t DDSFMT_A32B32G32R32F        = 116;
const int32_t DDSFMT_CxV8U8               = 117;

const int32_t DDSFMT_A1                   = 118;
const int32_t DDSFMT_A2B10G10R10_XR_BIAS  = 119;
const int32_t DDSFMT_BINARYBUFFER         = 199;

FormatDescriptor s_ddsFormats[] =
{
	{ DDSFMT_R8G8B8,		24, 0xFF0000,   0xFF00,	    0xFF,       0,				0, PIXEL_FORMAT_B8G8R8X8_UNORM	},
	{ DDSFMT_A8R8G8B8,		32, 0xFF0000,   0xFF00,     0xFF,       0xFF000000,		0, PIXEL_FORMAT_B8G8R8A8_UNORM	},  
	{ DDSFMT_X8R8G8B8,		32, 0xFF0000,   0xFF00,     0xFF,       0,				0, PIXEL_FORMAT_B8G8R8X8_UNORM	},           
	{ DDSFMT_R5G6B5,		16,	0xF800,     0x7E0,      0x1F,       0,				0, PIXEL_FORMAT_B5G6R5_UNORM	},
	{ DDSFMT_X1R5G5B5,		16, 0x7C00,     0x3E0,      0x1F,       0,				0, PIXEL_FORMAT_B5G5R5A1_UNORM	},
	{ DDSFMT_A1R5G5B5,		16, 0x7C00,     0x3E0,      0x1F,       0x8000,			0, PIXEL_FORMAT_B5G5R5A1_UNORM	},
	{ DDSFMT_A4R4G4B4,		16, 0xF00,      0xF0,       0xF,        0xF000,			0, PIXEL_FORMAT_UNKNOWN			},
	{ DDSFMT_R3G3B2,		8,  0xE0,       0x1C,       0x3,	    0,				0, PIXEL_FORMAT_UNKNOWN			},
	{ DDSFMT_A8,			8,  0,          0,          0,		    0xff,			0, PIXEL_FORMAT_A8_UNORM		},
	{ DDSFMT_A8R3G3B2,		16, 0xE0,       0x1C,       0x3,        0xFF00,			0, PIXEL_FORMAT_UNKNOWN			},
	{ DDSFMT_X4R4G4B4,		16, 0xF00,      0xF0,       0xF,        0,				0, PIXEL_FORMAT_UNKNOWN			},
	{ DDSFMT_A2B10G10R10,	32, 0x3FF,      0xFFC00,    0x3FF00000, 0xC0000000,		0, PIXEL_FORMAT_R10G10B10A2_UNORM },  
	{ DDSFMT_A8B8G8R8,		32, 0xFF,       0xFF00,     0xFF0000,   0xFF000000,		0, PIXEL_FORMAT_R8G8B8A8_UNORM	},  
	{ DDSFMT_X8B8G8R8,		32, 0xFF,       0xFF00,     0xFF0000,   0,				0, PIXEL_FORMAT_R8G8B8A8_UNORM	}, // no such thing as R8G8B8X8?
	{ DDSFMT_G16R16,		32, 0xFFFF,     0xFFFF0000, 0,          0,				0, PIXEL_FORMAT_R16G16_UNORM	},
	{ DDSFMT_A2R10G10B10,	32, 0x3FF00000, 0xFFC00,    0x3FF,      0xC0000000,		0, PIXEL_FORMAT_UNKNOWN			},

	{ DDSFMT_L8,			8,  0xff,       0,          0,          0,				0, PIXEL_FORMAT_R8_UNORM		},
	{ DDSFMT_L16,			16, 0xff,       0,          0,          0,				0, PIXEL_FORMAT_R16_UNORM		},

	{ DDSFMT_P8,			8,  0,          0,          0,          0,				0, PIXEL_FORMAT_UNKNOWN			},
	{ DDSFMT_A8L8,			16, 0xff,       0,          0,          0xff00,			0, PIXEL_FORMAT_R8G8_UNORM		},	// hack

	{ DDSFMT_R32F,			32, 0,			0,          0,          0,				114, PIXEL_FORMAT_R32_FLOAT },
	{ DDSFMT_R16F,			16, 0,			0,          0,          0,				111, PIXEL_FORMAT_R16_FLOAT },
	{ DDSFMT_A16B16G16R16,	64, 0,          0,          0,          0,				36,  PIXEL_FORMAT_R16G16B16A16_UNORM },
	{ DDSFMT_A16B16G16R16F, 64, 0,          0,          0,          0,				113, PIXEL_FORMAT_R16G16B16A16_FLOAT },
	{ DDSFMT_A32B32G32R32F, 128,0,          0,          0,          0,				116, PIXEL_FORMAT_R32G32B32A32_FLOAT },

	{ DDSFMT_UNKNOWN,		0,	0,			0,			0,			0,				0, PIXEL_FORMAT_UNKNOWN }
};

std::set<int32_t> s_supportedFormats;

typedef std::map<int32_t, const char*> FormatNameMap;
FormatNameMap s_formatNames;

#define ADD_NAMED_FORMAT( x ) s_formatNames[x] = #x


void CacheSupportedFormats()
{
	s_supportedFormats.clear();

	// TODO: Get this from the device
	s_supportedFormats.insert( DDSFMT_DXT1 );
	s_supportedFormats.insert( DDSFMT_DXT2 );
	s_supportedFormats.insert( DDSFMT_DXT3 );
	s_supportedFormats.insert( DDSFMT_DXT4 );
	s_supportedFormats.insert( DDSFMT_DXT5 );
	s_supportedFormats.insert( DDSFMT_DX10 );
	s_supportedFormats.insert( FOURCC_ATI1 );
	s_supportedFormats.insert( FOURCC_ATI2 );
	s_supportedFormats.insert( DDSFMT_X8R8G8B8 );
	s_supportedFormats.insert( DDSFMT_A8R8G8B8 );
	s_supportedFormats.insert( DDSFMT_A8B8G8R8 );
	s_supportedFormats.insert( DDSFMT_R5G6B5 );
	s_supportedFormats.insert( DDSFMT_X1R5G5B5 );
	s_supportedFormats.insert( DDSFMT_A1R5G5B5 );
	s_supportedFormats.insert( DDSFMT_A4R4G4B4 );
	s_supportedFormats.insert( DDSFMT_A8 );
	s_supportedFormats.insert( DDSFMT_A8L8 );
	s_supportedFormats.insert( DDSFMT_L8 );
	s_supportedFormats.insert( DDSFMT_G16R16 );
	s_supportedFormats.insert( DDSFMT_A16B16G16R16 );
	s_supportedFormats.insert( DDSFMT_A16B16G16R16F );
	s_supportedFormats.insert( DDSFMT_A32B32G32R32F );
	s_supportedFormats.insert( DDSFMT_R32F );
	s_supportedFormats.insert( DDSFMT_R16F );

	// Not supported by device but conversion done at load time (on background thread)
	s_supportedFormats.insert( DDSFMT_R8G8B8 );

}

class Initializer
{
public:
	Initializer()
	{
		CacheSupportedFormats();

		ADD_NAMED_FORMAT( DDSFMT_UNKNOWN             );
		ADD_NAMED_FORMAT( DDSFMT_R8G8B8              );
		ADD_NAMED_FORMAT( DDSFMT_A8R8G8B8            );
		ADD_NAMED_FORMAT( DDSFMT_X8R8G8B8            );
		ADD_NAMED_FORMAT( DDSFMT_R5G6B5              );
		ADD_NAMED_FORMAT( DDSFMT_X1R5G5B5            );
		ADD_NAMED_FORMAT( DDSFMT_A1R5G5B5            );
		ADD_NAMED_FORMAT( DDSFMT_A4R4G4B4            );
		ADD_NAMED_FORMAT( DDSFMT_R3G3B2              );
		ADD_NAMED_FORMAT( DDSFMT_A8                  );
		ADD_NAMED_FORMAT( DDSFMT_A8R3G3B2            );
		ADD_NAMED_FORMAT( DDSFMT_X4R4G4B4            );
		ADD_NAMED_FORMAT( DDSFMT_A2B10G10R10         );
		ADD_NAMED_FORMAT( DDSFMT_A8B8G8R8            );
		ADD_NAMED_FORMAT( DDSFMT_X8B8G8R8            );
		ADD_NAMED_FORMAT( DDSFMT_G16R16              );
		ADD_NAMED_FORMAT( DDSFMT_A2R10G10B10         );
		ADD_NAMED_FORMAT( DDSFMT_A16B16G16R16        );
		ADD_NAMED_FORMAT( DDSFMT_A8P8                );
		ADD_NAMED_FORMAT( DDSFMT_P8                  );
		ADD_NAMED_FORMAT( DDSFMT_L8                  );
		ADD_NAMED_FORMAT( DDSFMT_A8L8                );
		ADD_NAMED_FORMAT( DDSFMT_A4L4                );
		ADD_NAMED_FORMAT( DDSFMT_V8U8                );
		ADD_NAMED_FORMAT( DDSFMT_L6V5U5              );
		ADD_NAMED_FORMAT( DDSFMT_X8L8V8U8            );
		ADD_NAMED_FORMAT( DDSFMT_Q8W8V8U8            );
		ADD_NAMED_FORMAT( DDSFMT_V16U16              );
		ADD_NAMED_FORMAT( DDSFMT_A2W10V10U10         );
		ADD_NAMED_FORMAT( DDSFMT_UYVY                );
		ADD_NAMED_FORMAT( DDSFMT_R8G8_B8G8           );
		ADD_NAMED_FORMAT( DDSFMT_YUY2                );
		ADD_NAMED_FORMAT( DDSFMT_G8R8_G8B8           );
		ADD_NAMED_FORMAT( DDSFMT_DXT1                );
		ADD_NAMED_FORMAT( DDSFMT_DXT2                );
		ADD_NAMED_FORMAT( DDSFMT_DXT3                );
		ADD_NAMED_FORMAT( DDSFMT_DXT4                );
		ADD_NAMED_FORMAT( DDSFMT_DXT5                );
		ADD_NAMED_FORMAT( DDSFMT_DX10                );
		ADD_NAMED_FORMAT( DDSFMT_D16_LOCKABLE        );
		ADD_NAMED_FORMAT( DDSFMT_D32                 );
		ADD_NAMED_FORMAT( DDSFMT_D15S1               );
		ADD_NAMED_FORMAT( DDSFMT_D24S8               );
		ADD_NAMED_FORMAT( DDSFMT_D24X8               );
		ADD_NAMED_FORMAT( DDSFMT_D24X4S4             );
		ADD_NAMED_FORMAT( DDSFMT_D16                 );
		ADD_NAMED_FORMAT( DDSFMT_D32F_LOCKABLE       );
		ADD_NAMED_FORMAT( DDSFMT_D24FS8              );
		ADD_NAMED_FORMAT( DDSFMT_D32_LOCKABLE        );
		ADD_NAMED_FORMAT( DDSFMT_S8_LOCKABLE         );
		ADD_NAMED_FORMAT( DDSFMT_L16                 );
		ADD_NAMED_FORMAT( DDSFMT_VERTEXDATA          );
		ADD_NAMED_FORMAT( DDSFMT_INDEX16             );
		ADD_NAMED_FORMAT( DDSFMT_INDEX32             );
		ADD_NAMED_FORMAT( DDSFMT_Q16W16V16U16        );
		ADD_NAMED_FORMAT( DDSFMT_MULTI2_ARGB8        );
		ADD_NAMED_FORMAT( DDSFMT_R16F                );
		ADD_NAMED_FORMAT( DDSFMT_G16R16F             );
		ADD_NAMED_FORMAT( DDSFMT_A16B16G16R16F       );
		ADD_NAMED_FORMAT( DDSFMT_R32F                );
		ADD_NAMED_FORMAT( DDSFMT_G32R32F             );
		ADD_NAMED_FORMAT( DDSFMT_A32B32G32R32F       );
		ADD_NAMED_FORMAT( DDSFMT_CxV8U8              );
		ADD_NAMED_FORMAT( DDSFMT_A1                  );
		ADD_NAMED_FORMAT( DDSFMT_BINARYBUFFER        );
	}
};

Initializer s_initSupportedFormats;

	
bool IsCubeTexture( const DDS_HEADER& header )
{
	return header.dwCubemapFlags & DDS_CUBEMAP_ALLFACES ? true : false;
}

bool IsVolumeTexture( const DDS_HEADER& header )
{
	return header.dwHeaderFlags & DDS_HEADER_FLAGS_VOLUME ? true : false;
}

std::pair<int32_t, PixelFormat> FindDdsFormat( const DDS_PIXELFORMAT& pf, const PixelFormat dx10tr2pf )
{
	if( pf.dwFlags & DDS_FOURCC )
	{
		if( pf.dwFourCC == FOURCC_DXT1 )
		{
			return std::make_pair( (int32_t)pf.dwFourCC, PIXEL_FORMAT_BC1_UNORM );
		}
		if( pf.dwFourCC == FOURCC_DXT2 || pf.dwFourCC == FOURCC_DXT3 )
		{
			return std::make_pair( (int32_t)pf.dwFourCC, PIXEL_FORMAT_BC2_UNORM );
		}
		if( pf.dwFourCC == FOURCC_DXT4 || pf.dwFourCC == FOURCC_DXT5 )
		{
			return std::make_pair( (int32_t)pf.dwFourCC, PIXEL_FORMAT_BC3_UNORM );
		}
		if( pf.dwFourCC == FOURCC_ATI1 )
		{
			return std::make_pair( (int32_t)pf.dwFourCC, PIXEL_FORMAT_BC4_UNORM );
		}
		if( pf.dwFourCC == FOURCC_ATI2 )
		{
			return std::make_pair( (int32_t)pf.dwFourCC, PIXEL_FORMAT_BC5_UNORM );
		}
		if( pf.dwFourCC == FOURCC_DX10  )
		{
			return std::make_pair( (int32_t)pf.dwFourCC, dx10tr2pf );
		}
	}
	else if( pf.dwFlags & DDS_INDEXED )
	{
		return std::make_pair( DDSFMT_P8, PIXEL_FORMAT_UNKNOWN );
	}

	for (int i = 0; s_ddsFormats[i].format != DDSFMT_UNKNOWN; i++)
	{
		// Match by either FOURCC for "fat" formats or by bit mask/count
		if ( ( pf.dwFourCC && s_ddsFormats[i].fourCC == pf.dwFourCC ) || 
			( !pf.dwFourCC && 
			s_ddsFormats[i].bitcount == pf.dwRGBBitCount &&
			s_ddsFormats[i].rmask == pf.dwRBitMask &&
			s_ddsFormats[i].gmask == pf.dwGBitMask &&
			s_ddsFormats[i].bmask == pf.dwBBitMask &&
			s_ddsFormats[i].amask == pf.dwABitMask ) )
		{
			return std::make_pair( s_ddsFormats[i].format, s_ddsFormats[i].pixelFormat );
		}
	}

	return std::make_pair( DDSFMT_UNKNOWN, PIXEL_FORMAT_UNKNOWN );
}

ImageIO::Result CheckSupportedFormat( const DDS_HEADER& header,  const DDS_HEADER_DXT10& headerDxt10 )
{
	int32_t fmt = FindDdsFormat( header.ddspf, headerDxt10.dxgiFormat ).first;
	if( s_supportedFormats.find( fmt ) != s_supportedFormats.end() )
	{
		return ImageIO::Result::OK;
	}
	else
	{
		const char* fmtName = "<unknown>";
		FormatNameMap::iterator it = s_formatNames.find( fmt );
		if( it != s_formatNames.end() )
		{
			fmtName = it->second;
		}
		return ImageIO::Result( ImageIO::Result::HEADER_NOT_SUPPORTED, "unsupported DDS format %s", fmtName );
	}

}

void CopyHeaderValuesToMembers( const DDS_HEADER& header, const DDS_HEADER_DXT10& headerDxt10, ImageIO::BitmapDimensions& dimensions )
{
	PixelFormat format;
	uint32_t arraySize;
	if( header.ddspf.dwFourCC == FOURCC_DX10 )
	{
		format = headerDxt10.dxgiFormat;
		arraySize = headerDxt10.arraySize;
	}
	else
	{
		format = FindDdsFormat( header.ddspf, headerDxt10.dxgiFormat ).second;
		arraySize = 1;
	}
	uint32_t width = header.dwWidth;
	uint32_t height = header.dwHeight;
	uint32_t mipLevelCount = std::max( header.dwMipMapCount, 1u );

	if( IsCubeTexture( header ) )
	{
		dimensions = BitmapDimensions( TEX_TYPE_CUBE, format, width, width, 1, mipLevelCount );
	}
	else if( IsVolumeTexture( header ) )
	{
		uint32_t volumeDepth = header.dwDepth;
		dimensions = BitmapDimensions( TEX_TYPE_3D, format, width, height, volumeDepth, mipLevelCount, arraySize );
	}
	else
	{
		dimensions = BitmapDimensions( TEX_TYPE_2D, format, width, height, 1, mipLevelCount, arraySize );
	}
}

ImageIO::Result DoReadHeader( ICcpStream& stream, const ImageIO::LoadParameters& loadParameters, BitmapDimensions& dimensions, DDS_HEADER& header, DDS_HEADER_DXT10& headerDxt10, size_t& skipBytes )
{
	if( stream.Read( &header, sizeof( header ) ) == -1 )
	{
		return ImageIO::Result::READ_FAILURE;
	}

	// BeResMan->AddTextureDataRead( sizeof( m_header ) );

	if( header.dwFourCC != MAKEFOURCC('D', 'D', 'S', ' ') )
	{
		return ImageIO::Result::INVALID_HEADER;
	}

	if( header.ddspf.dwFourCC == FOURCC_DX10 )
	{
		if( stream.Read( &headerDxt10, sizeof( headerDxt10 ) ) == -1 )
		{
			return ImageIO::Result::READ_FAILURE;
		}
	}
	skipBytes = 0;

	// Skip ahead in the mip map chain for textures if so instructed.  Note that this
	// optimization won't work for Cube textures because of their data organization.
	if( !IsCubeTexture( header ) )
	{
		unsigned int skipCount = 0;
		unsigned int mipCount = ( header.dwHeaderFlags & DDS_HEADER_FLAGS_MIPMAP ) ? header.dwMipMapCount : 0;
		CopyHeaderValuesToMembers( header, headerDxt10, dimensions );
		loadParameters.GetMipLevelRange( header.dwWidth, header.dwHeight, skipCount, mipCount );
		
		if( skipCount )
		{
			// We first skip ahead in the stream to create the illusion of a texture
			// with fewer mip maps (lower res)
			if( FindDdsFormat( header.ddspf, headerDxt10.dxgiFormat ).first == DDSFMT_R8G8B8 )
			{
				for( unsigned int i = 0; i < skipCount; ++i )
				{
					skipBytes += dimensions.GetMipWidth( i ) * dimensions.GetMipHeight( i ) * dimensions.GetMipDepth( i ) * 3;
				}
			}
			else
			{
				for( unsigned int i = 0; i < skipCount; ++i )
				{
					skipBytes += dimensions.GetMipSize( i );
				}
			}
			
			// We now fudge the header to comply with these mip map adjustments
			header.dwMipMapCount = mipCount;
			header.dwWidth >>= skipCount;
			header.dwHeight >>= skipCount;
			header.dwDepth = std::max( 1u, header.dwDepth >> skipCount );
		}
	}

	CopyHeaderValuesToMembers( header, headerDxt10, dimensions );

	return ImageIO::Result::OK;
}

bool MakePixelFormat( DDS_PIXELFORMAT& ddspf, const BitmapDimensions& bd )
{
	const PixelFormat pixelFormat = bd.GetFormat();
	const bool bCompressed = IsCompressedFormat( pixelFormat );
	
	// Set the pixel format structure size to 32 (mandatory
	ddspf.dwSize = 32;
	// Set the pixel format for an uncompressed texture
	if( !bCompressed )
	{
		// Search for the correct format
		int i = 0;
		while( s_ddsFormats[i].format != DDSFMT_UNKNOWN )
		{
			if( s_ddsFormats[i].pixelFormat == pixelFormat && s_ddsFormats[i].bitcount == GetBytesPerPixel( bd.GetFormat() ) * 8 )
			{
				// Set flags
				if( s_ddsFormats[i].fourCC != 0 )
				{
					ddspf.dwFlags = DDS_FOURCC;
				}
				else
				{
					if( s_ddsFormats[i].format == DDSFMT_L8 )
					{
						ddspf.dwFlags = DDS_LUMINANCE;
					}
					else if( s_ddsFormats[i].amask != 0x0 || s_ddsFormats[i].fourCC != 0 )
					{
						ddspf.dwFlags = DDS_RGBA;
					}
					else
					{
						ddspf.dwFlags = DDS_RGB;
					}
				}

				// Set bit depth and masks
				ddspf.dwRGBBitCount = s_ddsFormats[i].bitcount;
				ddspf.dwRBitMask = s_ddsFormats[i].rmask;
				ddspf.dwGBitMask = s_ddsFormats[i].gmask;
				ddspf.dwBBitMask = s_ddsFormats[i].bmask;
				ddspf.dwABitMask = s_ddsFormats[i].amask;
				ddspf.dwFourCC = s_ddsFormats[i].fourCC;

				return true;
			}
			++i;
		}

		return false;	// uncompressed format that we don't recognize
	}
	// Set the pixel format for a compressed texture
	else
	{
		if( pixelFormat == PIXEL_FORMAT_BC1_TYPELESS   || 
			pixelFormat == PIXEL_FORMAT_BC1_UNORM      ||
			pixelFormat == PIXEL_FORMAT_BC1_UNORM_SRGB )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_DXT1;
		}		
		else 
		if( pixelFormat == PIXEL_FORMAT_BC2_TYPELESS	||
			pixelFormat == PIXEL_FORMAT_BC2_UNORM   	||
			pixelFormat == PIXEL_FORMAT_BC2_UNORM_SRGB	)
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_DXT3;
		}
		else 
		if( pixelFormat == PIXEL_FORMAT_BC3_TYPELESS	||
			pixelFormat == PIXEL_FORMAT_BC3_UNORM   	||
			pixelFormat == PIXEL_FORMAT_BC3_UNORM_SRGB	)
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_DXT5;
		}
#if 0
		else	// TODO legacy stuff
		if( pixelFormat_Legacy == D3DFMT_DXT1 )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_DXT1;
		}
		else if( pixelFormat_Legacy == D3DFMT_DXT2 )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_DXT2;
		}
		else if( pixelFormat_Legacy == D3DFMT_DXT3 )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_DXT3;
		}
		else if( pixelFormat_Legacy == D3DFMT_DXT4 )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_DXT4;
		}
		else if( pixelFormat_Legacy == D3DFMT_DXT5 )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_DXT5;
		}
#endif

		/*
		else if( fmt == FOURCC_RXGB )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_RXGB;
		}*/
		else if( pixelFormat == PIXEL_FORMAT_BC4_SNORM ||
			pixelFormat == PIXEL_FORMAT_BC4_TYPELESS ||
			pixelFormat == PIXEL_FORMAT_BC4_UNORM )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_ATI1;
		}
		else if( pixelFormat == PIXEL_FORMAT_BC5_SNORM ||
			pixelFormat == PIXEL_FORMAT_BC5_TYPELESS ||
			pixelFormat == PIXEL_FORMAT_BC5_UNORM )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_ATI2;
		}
		ddspf.dwRGBBitCount = 0;
		ddspf.dwRBitMask = 0;
		ddspf.dwGBitMask = 0;
		ddspf.dwBBitMask = 0;
		ddspf.dwABitMask = 0;
	}

	return true;
}



ImageIO::Result BuildHeaders( const BitmapDimensions& bd, DDS_HEADER& header, DDS_HEADER_DXT10& headerDxt10 )
{
	unsigned int mips = bd.GetTrueMipCount();

	const bool bCompressed = IsCompressedFormat( bd.GetFormat() );
	
	memset( &header, 0, sizeof( header ) );
	memset( &headerDxt10, 0, sizeof( headerDxt10 ) );

	// Set magic number & mandatory header size. dwFourCC is not part of the stored header size :|
	header.dwFourCC = MAKEFOURCC('D', 'D', 'S', ' ');
	static_assert( sizeof( header ) == 124 + 4, "DDS header size not correct" );
	header.dwSize = sizeof( header ) - 4;

	// Set flags for a compressed texture format
	//
	// Note: the DDS documentation says that compressed textures should use the LINEARSIZE flag
	//       and uncompressed textures should use the PITCH flag.  Some DDS files we load do not
	//       follow this convention (typically, uncompressed RGB textures won't specify either flag),
	//       so loading then saving a texture won't result in bitwise-identical files.  The differences
	//       are in the header.
	if( bCompressed )
	{
		header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_LINEARSIZE;
	}
	// Set flags for an uncompressed texture format
	else
	{
		header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_PITCH;
	}
	// Set flags for a texture with mipmaps
	if( mips > 1 )
	{
		header.dwHeaderFlags |= DDS_HEADER_FLAGS_MIPMAP;
	}

	// Set texture 
	header.dwWidth		= bd.GetWidth();
	header.dwHeight		= bd.GetHeight();
	header.dwDepth		= 0;
	header.dwMipMapCount	= mips;
	
	if( bd.GetType() == TEX_TYPE_CUBE )
	{
		header.dwCubemapFlags |= DDS_CUBEMAP_ALLFACES;
	}
	else if( bd.GetType() == TEX_TYPE_3D )
	{
		header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_VOLUME;
		header.dwDepth = bd.GetDepth();
	}

	if( !MakePixelFormat( header.ddspf, bd ) )
	{
		return ImageIO::Result::SAVE_NOT_SUPPORTED;
	}
	
	// Set the surface flags
	header.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;
	if( mips > 1)
	{
		header.dwSurfaceFlags |= DDS_SURFACE_FLAGS_MIPMAP;
	}

	// Figure out the linear data size (for compressed textures)
	if( bCompressed )
	{
		header.dwPitchOrLinearSize = bd.GetMipSize( 0 );
	}
	// Figure out the pitch (for uncompressed textures)
	else
	{
		header.dwPitchOrLinearSize = (header.ddspf.dwRGBBitCount * header.dwWidth) / 8;
	}

	PixelFormat format = bd.GetFormat();

	if( IsDds10Format( format ) || ( bd.GetArraySize() != 1 && bd.GetType() != TEX_TYPE_CUBE ) )
	{
		header.ddspf.dwFlags |= DDS_FOURCC;
		header.ddspf.dwFourCC = FOURCC_DX10;
		headerDxt10.dxgiFormat = format;
		headerDxt10.arraySize = bd.GetArraySize();

		TextureType texture_type = bd.GetType();
		switch(texture_type)
		{
			case TEX_TYPE_1D:
				headerDxt10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE1D;
				break;
			case TEX_TYPE_2D:
				headerDxt10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
				break;
			case TEX_TYPE_3D:
				headerDxt10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE3D;
				break;
			case TEX_TYPE_CUBE:
				headerDxt10.resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
				headerDxt10.miscFlag |= 0x4;
				break;
			case TEX_TYPE_TYPELESS:
				headerDxt10.resourceDimension = D3D10_RESOURCE_DIMENSION_UNKNOWN;
				break;
			default:
				return ImageIO::Result::SAVE_NOT_SUPPORTED; 
		}
		
	}
	return ImageIO::Result::OK;
}

void Convert24BitTo32Bit( const uint8_t* rgbData, ImageIO::HostBitmap& bitmap, size_t size, size_t offset )
{
	const unsigned newSize = unsigned( size );
	const unsigned oldSize = newSize - newSize / 4;

	const uint8_t* end = rgbData + oldSize;
	const uint8_t* src = rgbData;
	uint8_t* dst = reinterpret_cast<uint8_t*>( bitmap.GetRawData() ) + offset;
	uint8_t* dstEnd = dst + newSize;
	while( src < end )
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = 0;

		if( (dst >= dstEnd) && (src < end) )
		{
			break;
		}
	}
}

ImageIO::Result ReadRgb( ICcpStream& stream, ImageIO::HostBitmap& bitmap, size_t elementSkipBytes )
{
	unsigned size = unsigned( elementSkipBytes ? bitmap.GetArrayElementSize() : bitmap.GetRawDataSize() );

	unsigned unconvertedSize = size / 4 * 3;
	uint8_t* data = (uint8_t*)CCP_MALLOC( "Tr2DdsHandler/m_data", unconvertedSize );
	ON_BLOCK_EXIT( [&] { CCP_FREE( data ); } );

	if( !data )
	{
		return ImageIO::Result( ImageIO::Result::OUT_OF_MEMORY, "couldn't allocate %d bytes", unconvertedSize );
	}

	if( elementSkipBytes )
	{
		for( uint32_t i = 0; i < bitmap.GetArraySize(); ++i )
		{
			stream.Seek( elementSkipBytes, ICcpStream::SO_CURRENT );
			if( stream.Read( data, unconvertedSize ) == -1 )
			{
				return ImageIO::Result::READ_FAILURE;
			}

			Convert24BitTo32Bit( data, bitmap, size, i * size );
		}
	}
	else
	{
		if( stream.Read( data, unconvertedSize ) == -1 )
		{
			return ImageIO::Result::READ_FAILURE;
		}

		Convert24BitTo32Bit( data, bitmap, size, 0 );
	}
	return ImageIO::Result::OK;
}

ImageIO::Result ReadImagePixels( ICcpStream& stream, size_t elementSkipBytes, ImageIO::HostBitmap& bitmap, const DDS_HEADER& header, const DDS_HEADER_DXT10& headerDxt10 )
{
	IMAGE_IO_CR_RETURN_RESULT( CheckSupportedFormat( header, headerDxt10 ) );

	if( FindDdsFormat( header.ddspf, headerDxt10.dxgiFormat ).first == DDSFMT_R8G8B8 )
	{
		return ReadRgb( stream, bitmap, elementSkipBytes );
	}

	if( elementSkipBytes )
	{
		for( uint32_t i = 0; i < bitmap.GetArraySize(); ++i )
		{
			stream.Seek( elementSkipBytes, ICcpStream::SO_CURRENT );
			if( stream.Read( bitmap.GetRawData() + i * bitmap.GetArrayElementSize(), bitmap.GetArrayElementSize() ) == -1 )
			{
				return ImageIO::Result::READ_FAILURE;
			}
		}
	}
	else
	{
		if( stream.Read( bitmap.GetRawData(), bitmap.GetRawDataSize() ) == -1 )
		{
			return ImageIO::Result::READ_FAILURE;
		}
	}
	if( FindDdsFormat( header.ddspf, headerDxt10.dxgiFormat ).first == DDSFMT_A8L8 )
	{
		if( !bitmap.ConvertFormat( PIXEL_FORMAT_B8G8R8A8_UNORM ) )
		{
			return ImageIO::Result::ERROR_CONVERTING_FORMAT;
		}
	}
	return ImageIO::Result::OK;
}

}


namespace ImageIO
{
namespace Dds
{

// --------------------------------------------------------------------------------------
// Description:
//   Registers DDS handler with ImageIO.
// --------------------------------------------------------------------------------------
void RegisterHandler()
{
	static bool s_registered = false;
	if( !s_registered )
	{
		ImageFormatFunctions funcs = { &IsDdsExtension, &ReadImage, &IsSaveSupported, &Save };
		RegisterImageHandler( funcs );
		s_registered = true;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if provided extension (without leading dot) is DDS extension.
// Arguments:
//   ext - File extension
// Return Value:
//   true If provided extension is DDS extension
// --------------------------------------------------------------------------------------
bool IsDdsExtension( const wchar_t* ext )
{
	return ( ext[0] == L'd' || ext[0] == L'D' ) &&
		( ext[1] == L'd' || ext[1] == L'D' ) &&
		( ext[2] == L's' || ext[2] == L'S' ) &&
		ext[3] == 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reads DDS image from the stream.
// Arguments:
//   stream - Stream used for reading
//   loadParameters - various loading parameters
//   bitmap - (out) Destination bitmap
//   metadata - (out) Optional image metadata
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result ReadImage( ICcpStream& stream, const ImageIO::LoadParameters& loadParameters, ImageIO::HostBitmap& bitmap, ImageIO::Metadata* metadata )
{
	BitmapDimensions dimensions;
	DDS_HEADER header = DDS_HEADER();
	DDS_HEADER_DXT10 headerDxt10 = DDS_HEADER_DXT10();
	size_t skipBytes = 0;
	IMAGE_IO_CR_RETURN_RESULT( DoReadHeader( stream, loadParameters, dimensions, header, headerDxt10, skipBytes ) );

	if( metadata )
	{
		metadata->cutout = Cutout();
	}
			
	if( !bitmap.CreateFromBitmapDimensions( dimensions ) )
	{
		return Result::ERROR_CREATING_BITMAP;
	}
	auto r = ReadImagePixels( stream, skipBytes, bitmap, header, headerDxt10 );
	if( !r )
	{
		bitmap.Destroy();
		return r;
	}
	if( metadata )
	{
		ImageIO::LoadCcpMetadata( stream, *metadata );
	}

	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if saving an image into DDS format is supported.
// Arguments:
//   dimensions - Image dimensions/type/format
// Return Value:
//   Result of the operation (OK if image saving is supported)
// --------------------------------------------------------------------------------------
Result IsSaveSupported( const BitmapDimensions& bd )
{
	DDS_PIXELFORMAT ddspf;
	if( !MakePixelFormat( ddspf, bd ) )		
	{
		return Result::SAVE_NOT_SUPPORTED;
	}

	return Result::OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves a bitmap to DDS file.
// Arguments:
//   image - Bitmap to save
//   output - Destination stream
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result Save( const ImageIO::HostBitmap& image, ICcpStream& output, const Metadata* metadata )
{
	if( !image.IsValid() )
	{
		return Result::INVALID_BITMAP;
	}

	DDS_HEADER header;
	DDS_HEADER_DXT10 headerDxt10;
	IMAGE_IO_CR_RETURN_RESULT( BuildHeaders( image, header, headerDxt10 ) );

	output.Write( &header, sizeof( header ) );
	if( header.ddspf.dwFourCC == FOURCC_DX10 )
	{
		output.Write( &headerDxt10, sizeof( headerDxt10 ) );
	}
	output.Write( image.GetRawData(), image.GetRawDataSize() );

	if( metadata && !metadata->metadata.empty() )
	{
		SaveCcpMetadata( output, *metadata );
	}
	return Result::OK;
}

}
}
