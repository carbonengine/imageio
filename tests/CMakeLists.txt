# Copyright © 2021 CCP ehf.

project(ImageIOTest VERSION 1.0.0)

ccp_add_executable(ImageIOTest
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

find_package(GTest REQUIRED)


target_link_libraries(ImageIOTest PRIVATE CcpImageIO GTest::GTest GTest::Main)
if(APPLE)
    target_link_libraries(ImageIOTest PRIVATE "-framework CoreFoundation")
elseif(WIN32)
    target_link_options(ImageIOTest PRIVATE "/IGNORE:4049")
endif()

if(CMAKE_IMPORT_LIBRARY_SUFFIX)

    add_custom_command(TARGET ImageIOTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:ImageIOTest> $<TARGET_FILE_DIR:ImageIOTest>
            COMMAND_EXPAND_LISTS
            VERBATIM
    )

endif()

target_compile_definitions(ImageIOTest PRIVATE BLUEBUILD)

set_target_properties(ImageIOTest PROPERTIES FOLDER "Tests")

gtest_discover_tests(ImageIOTest myListOfTests)