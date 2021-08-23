project(ImageIO VERSION 1.0.0)

add_library(ImageIO STATIC
        CcpMetadata.cpp
        HostBitmap.cpp
        ImageIOResult.cpp
        ImageUtility.cpp
        PsdHandler.cpp
        StdAfx.cpp
        Tr2BmpHandler.cpp
        Tr2DdsHandler.cpp
        Tr2ImageHandler.cpp
        Tr2JpgHandler.cpp
        Tr2PngHandler.cpp
        Tr2TgaHandler.cpp
)
target_sources(ImageIO PRIVATE
    CcpMetadata.h
    HostBitmap.h
    ImageIOResult.h
    ImageUtility.h
    PsdHandler.h
    StdAfx.h
    Tr2BmpHandler.h
    Tr2DdsHandler.h
    Tr2ImageHandler.h
    Tr2JpgHandler.h
    Tr2PngHandler.h
    Tr2TgaHandler.h
)

target_precompile_headers(ImageIO PRIVATE StdAfx.h)
set_target_properties(ImageIO PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME ImageIO)
target_include_directories(ImageIO PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_precompile_headers(ImageIO PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)

target_link_libraries(ImageIO PUBLIC CcpCore JPEG::JPEG PNG::PNG ZLIB::ZLIB)
