#include "StdAfx.h"

const char* g_moduleName = "ImageIOTest";

int main( int argc, char **argv )
{
	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}

#ifdef __ANDROID__
#include <android/native_activity.h>
extern "C" void ANativeActivity_onCreate(ANativeActivity*, void*, size_t)
{
    const char* argv[1] = { "ImageIOTest" };
    main( 1, const_cast<char**>( argv ) );
}
#endif