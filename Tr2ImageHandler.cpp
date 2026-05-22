// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "Tr2ImageHandler.h"


#include "Tr2PngHandler.h"
#include "Tr2DdsHandler.h"
#include "Tr2JpgHandler.h"
#include "Tr2TgaHandler.h"
#include "Tr2BmpHandler.h"
#include "PsdHandler.h"
#include "VtaHandler.h"

namespace
{

CcpMutex s_registerMutex( "ImageIO", "s_registerMutex", 50 );

TrackableStdVector<ImageIO::ImageFormatFunctions>& GetImageHandlers()
{
	static TrackableStdVector<ImageIO::ImageFormatFunctions> s_imageHandlers( "ImageIO::GetImageHandlers" );
	return s_imageHandlers;
}

void RegisterImageIOHandlers()
{
	CcpAutoMutex lock( s_registerMutex );

	static bool s_registered = false;
	if( !s_registered )
	{
		ImageIO::Bmp::RegisterHandler();
		ImageIO::Dds::RegisterHandler();
		ImageIO::Jpeg::RegisterHandler();
		ImageIO::Png::RegisterHandler();
		ImageIO::Psd::RegisterHandler();
		ImageIO::Tga::RegisterHandler();
		ImageIO::Vta::RegisterHandler();
		s_registered = true;
	}
}

const wchar_t* GetExtension( const wchar_t* filename )
{
	auto dot = wcsrchr( filename, L'.' );
	return dot ? dot + 1 : filename + wcslen( filename );
}

const ImageIO::ImageFormatFunctions* GetImageHandler( const wchar_t* extension )
{
	RegisterImageIOHandlers();
	auto& handlers = GetImageHandlers();
	for( size_t i = 0; i < handlers.size(); ++i )
	{
		if( ( *handlers[i].checkExtension )( extension ) )
		{
			return &handlers[i];
		}
	}
	return nullptr;
}

}

namespace ImageIO
{

// --------------------------------------------------------------------------------------
// Description:
//   Registers a new image format handler.
// Arguments:
//   imageHandler - Pointers to image handling functions
// --------------------------------------------------------------------------------------
void RegisterImageHandler( const ImageFormatFunctions& imageHandler )
{
	GetImageHandlers().push_back( imageHandler );
}

// --------------------------------------------------------------------------------------
// Description:
//   Loads an image from stream into a bitmap.
// Arguments:
//   src - Stream used for reading
//   loadParameters - various loading parameters
//   bitmap - (out) Destination bitmap
//   metadata - (out) Optional image metadata
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result ReadImage( ICcpStream& src, const LoadParameters& loadParameters, HostBitmap& bitmap, ImageIO::Metadata* metadata )
{
	auto handler = GetImageHandler( GetExtension( loadParameters.m_filename.c_str() ) );
	if( !handler )
	{
		return Result::UNRECOGNIZED_IMAGE_TYPE;
	}
	if( !handler->readImage )
	{
		return Result::METHOD_NOT_SUPPORTED;
	}
	return ( *handler->readImage )( src, loadParameters, bitmap, metadata );
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if saving an image into particular format is supported.
// Arguments:
//   filename - Destination file name (used to determine image format)
//   dimensions - Image dimensions/type/format
// Return Value:
//   Result of the operation (OK if image saving is supported)
// --------------------------------------------------------------------------------------
Result IsSaveSupported( const wchar_t* filename, const BitmapDimensions& dimensions )
{
	auto handler = GetImageHandler( GetExtension( filename ) );
	if( !handler )
	{
		return Result::UNRECOGNIZED_IMAGE_TYPE;
	}
	if( !handler->isSaveSupported )
	{
		return Result::METHOD_NOT_SUPPORTED;
	}
	return ( *handler->isSaveSupported )( dimensions );
}

// --------------------------------------------------------------------------------------
// Description:
//   Save a bitmap into a stream.
// Arguments:
//   filename - Destination file name (used to determine image format)
//   bitmap - Bitmap to save
//   dest - Destination stream
//   metadata - Optional image metadata
// Return Value:
//   Result of the operation
// --------------------------------------------------------------------------------------
Result SaveImage( const wchar_t* filename, const HostBitmap& bitmap, ICcpStream& dest, const Metadata* metadata )
{
	auto handler = GetImageHandler( GetExtension( filename ) );
	if( !handler )
	{
		return Result::UNRECOGNIZED_IMAGE_TYPE;
	}
	if( !handler->save )
	{
		return Result::METHOD_NOT_SUPPORTED;
	}
	return ( *handler->save )( bitmap, dest, metadata );
}

}


ImageIO::LoadParameters::LoadParameters( const wchar_t* filename, uint32_t mipLevelSkipCount, uint32_t mipLevelMaxCount )
	:m_filename( filename ),
	m_mipLevelSkipCount( mipLevelSkipCount ),
	m_mipLevelMaxCount( mipLevelMaxCount )
{
}

void ImageIO::LoadParameters::GetMipLevelRange( uint32_t width, uint32_t height, uint32_t& skipCount, uint32_t& mipCount ) const
{
	skipCount = 0;
	
	if( width > 8 && height > 8 )
	{
		if( mipCount > m_mipLevelSkipCount )
		{
			skipCount = m_mipLevelSkipCount;
		}
		else if( mipCount )
		{
			// limit total mipmap count to three!
			skipCount = mipCount >= 3 ? mipCount - 3 : 0;
		}
		else
		{
			// mipCount == 0 is a special case where the driver is asked to autogenerate the mip maps.
			// We cannot skip the first n in the chain because we don't have a chain to load!
		}
	}

	mipCount -= skipCount;
	if( mipCount > m_mipLevelMaxCount )
	{
		skipCount += (mipCount - m_mipLevelMaxCount);
		mipCount = m_mipLevelMaxCount;
	}
}

