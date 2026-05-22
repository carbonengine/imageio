// Copyright © 2014 CCP ehf.

#pragma once
#ifndef ImageIOResult_H
#define ImageIOResult_H

namespace ImageIO
{

struct Result
{
	enum Code
	{
		OK,
		UNRECOGNIZED_IMAGE_TYPE,
		METHOD_NOT_SUPPORTED,
		READ_FAILURE,
		WRITE_FAILURE,
		INVALID_HEADER,
		HEADER_NOT_SUPPORTED,
		INVALID_DATA,
		ERROR_CREATING_BITMAP,
		INVALID_BITMAP,
		SAVE_NOT_SUPPORTED,
		OUT_OF_MEMORY,
		ERROR_CONVERTING_FORMAT,
		ERROR_INITIALIZING_EXTERNAL_LIBRARY,
		UNKNOWN_FAILURE,
	};

	Code code;
	std::string message;

	Result()
		:code( OK )
	{
	}

	Result( Code c )
		:code( c )
	{
	}

	Result( Code code, const char* format, ... );
	static Result FormatVAList( Code code, const char* format, va_list );

	operator bool() const
	{
		return code == OK;
	}

	std::string GetErrorMessage() const;
};

#define IMAGE_IO_CR_RETURN_RESULT( x ) { auto _imageiores = ( x ); if( !_imageiores ) { return _imageiores; } }

}

#endif