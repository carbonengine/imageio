#include "StdAfx.h"

#include "Tr2DdsHandler.h"
#include "HostBitmap.h"

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

using namespace Tr2RenderContextEnum;

extern bool g_convertA8L8FormatToB8G8R8A8;

namespace
{

	const unsigned int FOURCC_DDS = MAKEFOURCC('D', 'D', 'S', ' ');
	const unsigned int FOURCC_DXT1 = MAKEFOURCC('D', 'X', 'T', '1');
	const unsigned int FOURCC_DXT2 = MAKEFOURCC('D', 'X', 'T', '2');
	const unsigned int FOURCC_DXT3 = MAKEFOURCC('D', 'X', 'T', '3');
	const unsigned int FOURCC_DXT4 = MAKEFOURCC('D', 'X', 'T', '4');
	const unsigned int FOURCC_DXT5 = MAKEFOURCC('D', 'X', 'T', '5');
	const unsigned int FOURCC_RXGB = MAKEFOURCC('R', 'X', 'G', 'B');
	const unsigned int FOURCC_ATI1 = MAKEFOURCC('A', 'T', 'I', '1');
	const unsigned int FOURCC_ATI2 = MAKEFOURCC('A', 'T', 'I', '2');

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
		{ DDSFMT_R8G8B8,		24, 0xFF0000,   0xFF00,	    0xFF,       0,				0, PIXEL_FORMAT_UNKNOWN			},
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

		{ DDSFMT_A16B16G16R16,	64, 0,          0,          0,          0,				36,  PIXEL_FORMAT_R16G16B16A16_UNORM },
		{ DDSFMT_A16B16G16R16F, 64, 0,          0,          0,          0,				113, PIXEL_FORMAT_R16G16B16A16_FLOAT },
		{ DDSFMT_A32B32G32R32F, 128,0,          0,          0,          0,				116, PIXEL_FORMAT_R32G32B32A32_FLOAT },

		{ DDSFMT_UNKNOWN,		0,	0,			0,			0,			0,				0, PIXEL_FORMAT_UNKNOWN }
	};

	std::set<int32_t> s_supportedFormats;
	
	typedef std::map<int32_t, const char*> FormatNameMap;
	FormatNameMap s_formatNames;

	#define ADD_NAMED_FORMAT( x ) s_formatNames[x] = #x

	class Initializer
	{
	public:
		Initializer()
		{
			Tr2DdsHandler::CacheSupportedFormats();

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

}

Tr2DdsHandler::Tr2DdsHandler( const wchar_t* sourceName ) 
	: Tr2ImageHandler( sourceName )
	, m_fullMipSize( 0 )
	, m_format( PIXEL_FORMAT_UNKNOWN )
{
	memset( &m_header, 0, sizeof( m_header ) );
}

Tr2DdsHandler::~Tr2DdsHandler()
{
}

bool Tr2DdsHandler::ReadHeader( ICcpStream* stream )
{
	if( !stream || stream->Read( &m_header, sizeof( m_header ) ) == -1 )
	{
		return false;
	}

	// BeResMan->AddTextureDataRead( sizeof( m_header ) );

	if( m_header.dwFourCC != MAKEFOURCC('D', 'D', 'S', ' ') )
	{
		return false;
	}

	// Skip ahead in the mip map chain for textures if so instructed.  Note that this
	// optimization won't work for Cube textures because of their data organization.
	if( !IsCubeTexture() )
	{
		unsigned int skipCount = 0;
		unsigned int mipCount = ( m_header.dwHeaderFlags & DDS_HEADER_FLAGS_MIPMAP ) ? m_header.dwMipMapCount : 0;
		CopyHeaderValuesToMembers();
		GetMipLevelRange( skipCount, mipCount );
		
		if( skipCount )
		{
			// We first skip ahead in the stream to create the illusion of a texture
			// with fewer mip maps (lower res)
			unsigned int skipBytes = 0;
			CopyHeaderValuesToMembers();
			for( unsigned int i = 0; i < skipCount; ++i )
			{
				skipBytes += GetMipLevelSize( i );
			}
			stream->Seek( skipBytes, ICcpStream::SO_CURRENT );
			
			// We now fudge the header to comply with these mip map adjustments
			m_header.dwMipMapCount = mipCount;
			m_header.dwWidth >>= skipCount;
			m_header.dwHeight >>= skipCount;
			
		}
	}

	CopyHeaderValuesToMembers();
	return true;
}

void Tr2DdsHandler::CopyHeaderValuesToMembers()
{
	m_format = FindDdsFormat( m_header.ddspf ).second;

	m_width			= m_header.dwWidth;
	m_height		= m_header.dwHeight;

	// "Fat" formats define themselves as FOURCC so we derive bits per pixel
	// from format
	if( ( m_header.ddspf.dwFlags & DDS_FOURCC ) && ( m_header.ddspf.dwFourCC < 256 ) )
	{
		if( m_format != PIXEL_FORMAT_UNKNOWN )
		{
			m_bitsPerPixel = Tr2RenderContextEnum::GetBytesPerPixel( m_format ) * 8;
		}
		else
		{
			m_bitsPerPixel = 0;
		}
	}
	else
	{
		m_bitsPerPixel	= m_header.ddspf.dwRGBBitCount;
	}
	m_mipLevelCount = std::max( m_header.dwMipMapCount, 1u );

	if( m_header.dwHeaderFlags & DDS_HEADER_FLAGS_VOLUME )
	{
		m_volumeDepth = m_header.dwDepth;
	}
	else
	{
		m_volumeDepth = 1;
	}

	m_fullMipSize = 0;
	for( unsigned i = 0; i != m_mipLevelCount; ++i )
	{
		m_fullMipSize += GetMipLevelSize( i );
	}
}

bool Tr2DdsHandler::MakePixelFormat( DDS_PIXELFORMAT &ddspf, const Tr2BitmapDimensions& bd )
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
			if( s_ddsFormats[i].pixelFormat == pixelFormat )
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
		else	// TODO legacy crap
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
		}
		else if( fmt == FOURCC_ATI1 )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_ATI1;
		}
		else if( fmt == FOURCC_ATI2 )
		{
			ddspf.dwFlags |= DDS_FOURCC;
			ddspf.dwFourCC = FOURCC_ATI2;
		}*/
		ddspf.dwRGBBitCount = 0;
		ddspf.dwRBitMask = 0;
		ddspf.dwGBitMask = 0;
		ddspf.dwBBitMask = 0;
		ddspf.dwABitMask = 0;
	}

	return true;
}

bool Tr2DdsHandler::IsSaveSupported( const Tr2BitmapDimensions& bd )
{
	Tr2DdsHandler::DDS_PIXELFORMAT ddspf;
	if( !Tr2DdsHandler::MakePixelFormat( ddspf, bd ) )		
	{
		CCP_LOGERR( "Texture has a unsupported pixelformat %d", bd.GetFormat() );
		return false;
	}

	return true;
}

bool Tr2DdsHandler::BuildHeader( const Tr2BitmapDimensions& bd )
{
	unsigned int mips = bd.GetTrueMipCount();

	const bool bCompressed = Tr2RenderContextEnum::IsCompressedFormat( bd.GetFormat() );
	
	memset( &m_header, 0, sizeof( m_header ) );

	// Set magic number & mandatory header size. dwFourCC is not part of the stored header size :|
	m_header.dwFourCC = MAKEFOURCC('D', 'D', 'S', ' ');
	static_assert( sizeof( m_header ) == 124 + 4, "DDS header size not correct" );
	m_header.dwSize = sizeof( m_header ) - 4;

	// Set flags for a compressed texture format
	//
	// Note: the DDS documentation says that compressed textures should use the LINEARSIZE flag
	//       and uncompressed textures should use the PITCH flag.  Some DDS files we load do not
	//       follow this convention (typically, uncompressed RGB textures won't specify either flag),
	//       so loading then saving a texture won't result in bitwise-identical files.  The differences
	//       are in the header.
	if( bCompressed )
	{
		m_header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_LINEARSIZE;
	}
	// Set flags for an uncompressed texture format
	else
	{
		m_header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_PITCH;
	}
	// Set flags for a texture with mipmaps
	if( mips > 1 )
	{
		m_header.dwHeaderFlags |= DDS_HEADER_FLAGS_MIPMAP;
	}

	// Set texture 
	m_header.dwWidth		= bd.GetWidth();
	m_header.dwHeight		= bd.GetHeight();
	m_header.dwDepth		= 0;
	m_header.dwMipMapCount	= mips;
	
	if( bd.GetType() == TEX_TYPE_CUBE )
	{
		m_header.dwCubemapFlags |= DDS_CUBEMAP_ALLFACES;
	}
	else if( bd.GetType() == TEX_TYPE_3D )
	{
		m_header.dwHeaderFlags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_VOLUME;
		m_header.dwDepth = bd.GetDepth();
	}

	if( !MakePixelFormat( m_header.ddspf, bd ) )
	{
		return false;
	}
	
	// Set the surface flags
	m_header.dwSurfaceFlags = DDS_SURFACE_FLAGS_TEXTURE;
	if( mips > 1)
	{
		m_header.dwSurfaceFlags |= DDS_SURFACE_FLAGS_MIPMAP;
	}

	// Figure out the linear data size (for compressed textures)
	if( bCompressed )
	{
		m_header.dwPitchOrLinearSize = bd.GetMipSize( 0 );
	}
	// Figure out the pitch (for uncompressed textures)
	else
	{
		m_header.dwPitchOrLinearSize = (m_header.ddspf.dwRGBBitCount * m_header.dwWidth) / 8;
	}

	CopyHeaderValuesToMembers();
	return true;
}

bool Tr2DdsHandler::ReadImage( ICcpStream* stream )
{
	unsigned int size = GetTotalDataSize();

	if( size == 0 || !stream || !IsSupported() )
	{
		return false;
	}

	m_data = (uint8_t*)CCP_MALLOC( "Tr2DdsHandler/m_data", size );

    if( !m_data )
    {
		CCP_LOGERR( "Tr2DdsHandler couldn't allocate %d bytes (%S)", size, m_sourceName.c_str() );
        return false;
    }

	if( stream->Read( m_data, size ) == -1 )
	{
		CCP_LOGERR( "Tr2DdsHandler couldn't read %d bytes (%S)", size, m_sourceName.c_str() );
		CCP_FREE( m_data );
		m_data = 0;
		return false;
	}

	// BeResMan->AddTextureDataRead( size );

	if( FindDdsFormat( m_header.ddspf ).first == DDSFMT_R8G8B8 )
	{
		if( !Convert24BitTo32Bit() )
		{
			CCP_LOGERR( "Tr2DdsHandler couldn't convert 24bit to 32bit (%S)", m_sourceName.c_str() );
			return false;
		}
	}

	if( g_convertA8L8FormatToB8G8R8A8 && FindDdsFormat( m_header.ddspf ).first == DDSFMT_A8L8 )
	{
		m_desiredFormat = PIXEL_FORMAT_B8G8R8A8_UNORM;	// decompress on the CPU, L8A8 no longer exists, and can't easily patch shaders to support both
		if( !ConvertDesiredFormat() )
		{
			return false;
		}

		DDS_PIXELFORMAT& pf = m_header.ddspf;
		
		pf.dwRGBBitCount	= 32;
		pf.dwRBitMask		= 0x00FF0000u;
		pf.dwGBitMask		= 0x0000FF00u;
		pf.dwBBitMask		= 0x000000FFu;
		pf.dwABitMask		= 0xFF000000u;
		
		CopyHeaderValuesToMembers();
	}

	if( (m_desiredFormat != PIXEL_FORMAT_UNKNOWN) && ConvertDesiredFormat() )
	{
		m_format = m_desiredFormat;
	}

	return true;
}

std::pair<int32_t, PixelFormat> Tr2DdsHandler::FindDdsFormat( const DDS_PIXELFORMAT& pf )
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

unsigned Tr2DdsHandler::GetBlockByteSize() const
{
	switch( m_header.ddspf.dwFourCC )
	{
		case FOURCC_DXT1:
		case FOURCC_ATI1:
			return 8;
		case FOURCC_DXT2:
		case FOURCC_DXT3:
		case FOURCC_DXT4:
		case FOURCC_DXT5:
		case FOURCC_RXGB:
		case FOURCC_ATI2:
			return 16;
	};

	// Not a block image.
	return 0;
}

bool Tr2DdsHandler::IsCubeTexture() const
{
	return m_header.dwCubemapFlags & DDS_CUBEMAP_ALLFACES ? true : false;
}

bool Tr2DdsHandler::IsVolumeTexture() const
{
	return m_header.dwHeaderFlags & DDS_HEADER_FLAGS_VOLUME ? true : false;
}

bool Tr2DdsHandler::IsSupported() const
{
	int32_t fmt = FindDdsFormat( m_header.ddspf ).first;
	if( s_supportedFormats.find( fmt ) != s_supportedFormats.end() )
	{
		return true;
	}
	else
	{
		const char* fmtName = "<unknown>";
		FormatNameMap::iterator it = s_formatNames.find( fmt );
		if( it != s_formatNames.end() )
		{
			fmtName = it->second;
		}
		CCP_LOGWARN( "Tr2DdsHandler: '%S' in unsupported format (%s)\n", m_sourceName.c_str(), fmtName );
		return false;
	}

}

void Tr2DdsHandler::CacheSupportedFormats()
{
	s_supportedFormats.clear();

	// TODO: Get this from the device
	s_supportedFormats.insert( DDSFMT_DXT1 );
	s_supportedFormats.insert( DDSFMT_DXT2 );
	s_supportedFormats.insert( DDSFMT_DXT3 );
	s_supportedFormats.insert( DDSFMT_DXT4 );
	s_supportedFormats.insert( DDSFMT_DXT5 );
	s_supportedFormats.insert( DDSFMT_X8R8G8B8 );
	s_supportedFormats.insert( DDSFMT_A8R8G8B8 );
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

	// Not supported by device but conversion done at load time (on background thread)
	s_supportedFormats.insert( DDSFMT_R8G8B8 );

}

Tr2RenderContextEnum::PixelFormat Tr2DdsHandler::GetFormat() const
{
	return m_format;
}

bool Tr2DdsHandler::Convert24BitTo32Bit()
{
	if( !Tr2ImageHandler::Convert24BitTo32Bit() )
	{
		return false;
	}
			
	m_header.ddspf.dwRGBBitCount = 32;

	m_fullMipSize = 0;
	for( unsigned i = 0; i != m_mipLevelCount; ++i )
	{
		m_fullMipSize += GetMipLevelSize( i );
	}

	CopyHeaderValuesToMembers();
	return true;
}

unsigned Tr2DdsHandler::GetOffset( unsigned mipLevel, unsigned face ) const
{
	unsigned offset = 0;
	for( unsigned int i = 0; i != mipLevel ; ++i )
	{
		offset += GetMipLevelSize( i );
	}
	offset += m_fullMipSize * face;
	return offset;
}

bool Tr2DdsHandler::Save( const ImageIO::HostBitmap& image, ICcpStream* output )
{
	CCP_ASSERT( output );
	CCP_ASSERT( image.IsValid() );

	if( !BuildHeader( image ) )
	{
		return false;
	}

	output->Write( &m_header, sizeof( m_header ) );
	output->Write( image.GetRawData(), image.GetRawDataSize() );
	return true;
}

