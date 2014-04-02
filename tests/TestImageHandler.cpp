#include "StdAfx.h"

using namespace ImageIO;

TEST( ImageHandler, CreateImageHandlerReturnsBmpHandlerForBmpExtension )
{
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( L"test.bmp" ) );
	ASSERT_NE( nullptr, dynamic_cast<Tr2BmpHandler*>( handler.get() ) );
}

TEST( ImageHandler, CreateImageHandlerReturnsDdsHandlerForDdsExtension )
{
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( L"test.dds" ) );
	ASSERT_NE( nullptr, dynamic_cast<Tr2DdsHandler*>( handler.get() ) );
}

TEST( ImageHandler, CreateImageHandlerReturnsJpgHandlerForJpgExtension )
{
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( L"test.jpg" ) );
	ASSERT_NE( nullptr, dynamic_cast<Tr2JpgHandler*>( handler.get() ) );
}

TEST( ImageHandler, CreateImageHandlerReturnsJpgHandlerForJpegExtension )
{
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( L"test.jpeg" ) );
	ASSERT_NE( nullptr, dynamic_cast<Tr2JpgHandler*>( handler.get() ) );
}

TEST( ImageHandler, CreateImageHandlerReturnsPngHandlerForPngExtension )
{
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( L"test.png" ) );
	ASSERT_NE( nullptr, dynamic_cast<Tr2PngHandler*>( handler.get() ) );
}

TEST( ImageHandler, CreateImageHandlerReturnsTgaHandlerForTgaExtension )
{
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( L"test.tga" ) );
	ASSERT_NE( nullptr, dynamic_cast<Tr2TgaHandler*>( handler.get() ) );
}

TEST( ImageHandler, CreateImageHandlerReturnsTgaHandlerForPsdExtension )
{
	std::unique_ptr<Tr2ImageHandler> handler( CreateImageHandler( L"test.psd" ) );
	ASSERT_NE( nullptr, dynamic_cast<ImageIO::PsdHandler*>( handler.get() ) );
}
