project(ImageIOTest VERSION 1.0.0)

add_executable(ImageIOTest
        ImageIOTest.cpp
        MemoryStream.cpp
        MemoryStream.h
        StdAfx.cpp
        StdAfx.h
        TestBmpHandler.cpp
        TestDdsHandler.cpp
        TestHelpers.cpp
        TestHelpers.h
        TestHostBitmap.cpp
        TestJpgHandler.cpp
        TestPngHandler.cpp
        TestPsdHandler.cpp
        TestTgaHandler.cpp
        )

target_precompile_headers(ImageIOTest PRIVATE StdAfx.h)

target_include_directories(ImageIOTest 
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. 
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_precompile_headers(ImageIOTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)

target_link_libraries(ImageIOTest PRIVATE ImageIO GTest::GTest GTest::Main)
if(APPLE)
    target_link_libraries(ImageIOTest PRIVATE "-framework CoreFoundation")
elseif(WIN32)
    target_link_options(ImageIOTest PRIVATE "/IGNORE:4049")
endif()

target_compile_definitions(ImageIOTest PRIVATE BLUEBUILD)

set_target_properties(ImageIOTest PROPERTIES FOLDER "Tests")