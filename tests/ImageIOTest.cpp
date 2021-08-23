#include "StdAfx.h"

const char* g_moduleName = "ImageIOTest";

int main( int argc, char **argv )
{
	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}
