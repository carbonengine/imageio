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


target_link_libraries(ImageIOTest PRIVATE ImageIO GTest::GTest GTest::Main)
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

    #Manually run applocal force no debug in config to copy dlls for libjpeg-turbo and libpng
    add_custom_command(TARGET ImageIOTest POST_BUILD
        COMMAND "${Z_VCPKG_POWERSHELL_PATH}" -noprofile -executionpolicy Bypass -file "${Z_VCPKG_TOOLCHAIN_DIR}/msbuild/applocal.ps1"
        -targetBinary "$<TARGET_FILE:ImageIOTest>"
        -installedDir "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin"
        VERBATIM
    )

endif()

target_compile_definitions(ImageIOTest PRIVATE BLUEBUILD)

set_target_properties(ImageIOTest PROPERTIES FOLDER "Tests")

gtest_discover_tests(ImageIOTest myListOfTests)