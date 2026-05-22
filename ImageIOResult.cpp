// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "ImageIOResult.h"

namespace
{

std::string GetResultErrorMessage( ImageIO::Result::Code code )
{
	switch( code )
	{
	case ImageIO::Result::OK:
		return "operation completed successfully";
	case ImageIO::Result::UNRECOGNIZED_IMAGE_TYPE:
		return "unrecognized image type";
	case ImageIO::Result::METHOD_NOT_SUPPORTED:
		return "method not supported for this image type";
	case ImageIO::Result::READ_FAILURE:
		return "error reading from stream";
	case ImageIO::Result::WRITE_FAILURE:
		return "error writing to stream";
	case ImageIO::Result::INVALID_HEADER:
		return "invalid image header";
	case ImageIO::Result::HEADER_NOT_SUPPORTED:
		return "image header not supported";
	case ImageIO::Result::INVALID_DATA:
		return "invalid data in input stream";
	case ImageIO::Result::ERROR_CREATING_BITMAP:
		return "error creating host bitmap";
	case ImageIO::Result::INVALID_BITMAP:
		return "invalid bitmap passed for saving";
	case ImageIO::Result::SAVE_NOT_SUPPORTED:
		return "saving of this bitmap is not supported";
	case ImageIO::Result::OUT_OF_MEMORY:
		return "out of memory";
	case ImageIO::Result::ERROR_CONVERTING_FORMAT:
		return "error converting pixel format";
	case ImageIO::Result::ERROR_INITIALIZING_EXTERNAL_LIBRARY:
		return "error initializing external library";
	case ImageIO::Result::UNKNOWN_FAILURE:
	default:
		return "unknown failure";
	}
}

}

ImageIO::Result::Result( Code code_, const char* format, ... )
{
	code = code_;

	va_list args;
	va_start( args, format );
	size_t size = size_t( _vscprintf( format, args ) );
	std::unique_ptr<char[]> buffer( CCP_NEW( "ImageIO::Result::buffer" ) char[size + 1] );
	vsprintf_s( buffer.get(), size + 1, format, args );
	message = buffer.get();
}

ImageIO::Result ImageIO::Result::FormatVAList( Code code, const char* format, va_list args )
{
	size_t size = size_t( _vscprintf( format, args ) );
	std::unique_ptr<char[]> buffer( CCP_NEW( "ImageIO::Result::buffer" ) char[size + 1] );
	vsprintf_s( buffer.get(), size + 1, format, args );

	Result result;
	result.code = code;
	result.message = buffer.get();
	return result;
}

std::string ImageIO::Result::GetErrorMessage() const
{
	std::string result = GetResultErrorMessage( code );
	if( !message.empty() )
	{
		result += ": " + message;
	}
	return result;
}
