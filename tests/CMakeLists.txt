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

# Copy binary runtime dependencies on DLL platforms
find_package(Tracy REQUIRED NO_CMAKE_PATH)
add_custom_command(TARGET ImageIOTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:ImageIOTest>
    $<TARGET_FILE_DIR:ImageIOTest>
)

gtest_discover_tests(ImageIOTest myListOfTests)

if(WIN32)
    find_package(TrinityAL_stub REQUIRED NO_CMAKE_PATH)
    target_include_directories(ImageIOTest PRIVATE "$<TARGET_PROPERTY:TrinityAL_stub,INTERFACE_INCLUDE_DIRECTORIES>")
elseif(APPLE)
    find_package(TrinityAL_metal REQUIRED NO_CMAKE_PATH)
    target_include_directories(ImageIOTest PRIVATE "$<TARGET_PROPERTY:TrinityAL_metal,INTERFACE_INCLUDE_DIRECTORIES>")
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

target_link_libraries(ImageIOTest PRIVATE ImageIO GTest::GTest GTest::Main)
if(APPLE)
    target_link_libraries(ImageIOTest PRIVATE "-framework CoreFoundation")
elseif(WIN32)
    target_link_options(ImageIOTest PRIVATE "/IGNORE:4049")
endif()

target_compile_definitions(ImageIOTest PRIVATE BLUEBUILD)

set_target_properties(ImageIOTest PROPERTIES FOLDER "Tests")