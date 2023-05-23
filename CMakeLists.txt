cmake_minimum_required(VERSION 3.16)
project(ImageIO VERSION 1.0.0)

if(NOT DEFINED ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "Missing required environment variable CCP_EVE_PERFORCE_BRANCH_PATH")
elseif(NOT IS_ABSOLUTE $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be an absolute path")
elseif(NOT EXISTS $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH is pointing at a non-existing location $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
elseif(NOT IS_DIRECTORY $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be a directory")
else()
    message(STATUS "Using CCP_EVE_PERFORCE_BRANCH_PATH located at $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
endif()

file(TO_CMAKE_PATH "$ENV{CCP_EVE_PERFORCE_BRANCH_PATH}" BRANCH_ROOT_DIR)
list(INSERT CMAKE_MODULE_PATH 0 ${BRANCH_ROOT_DIR}/cmake)
include(CcpGlobalSettings)
include(CcpPackageConfigHelpers)

ccp_add_library(ImageIO STATIC
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

set(PUBLIC_HEADER_FILES 
    include/CcpMetadata.h
    include/HostBitmap.h
    include/ImageIOResult.h
    include/ImageUtility.h
    include/PsdHandler.h
    include/Tr2BmpHandler.h
    include/Tr2DdsHandler.h
    include/Tr2ImageHandler.h
    include/Tr2JpgHandler.h
    include/Tr2PngHandler.h
    include/Tr2TgaHandler.h
)

target_sources(ImageIO PRIVATE ${PUBLIC_HEADER_FILES})

target_precompile_headers(ImageIO PRIVATE StdAfx.h)
set_target_properties(ImageIO PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME ImageIO)

target_precompile_headers(ImageIO PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)

find_package(JPEG REQUIRED NO_CMAKE_PATH)
find_package(PNG REQUIRED NO_CMAKE_PATH)
find_package(ZLIB REQUIRED NO_CMAKE_PATH)
find_package(CcpCore REQUIRED NO_CMAKE_PATH)

# TrinityAL was not previously linked, however ImageIO stdAfx was referencing some headers from TrinityAL
if(WIN32)
    find_package(TrinityAL_stub REQUIRED NO_CMAKE_PATH)
elseif(APPLE)
    find_package(TrinityAL_metal REQUIRED NO_CMAKE_PATH)
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

target_include_directories(ImageIO PUBLIC "$<$<BOOL:${WIN32}>:$<TARGET_PROPERTY:TrinityAL_stub,INTERFACE_INCLUDE_DIRECTORIES>>")
target_include_directories(ImageIO PUBLIC "$<$<BOOL:${APPLE}>:$<TARGET_PROPERTY:TrinityAL_metal,INTERFACE_INCLUDE_DIRECTORIES>>")

target_include_directories(ImageIO
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ImageIO PUBLIC CcpCore JPEG::JPEG PNG::PNG ZLIB::ZLIB)

target_compile_options(ImageIO PRIVATE "${MATH_OPTIMIZE_FLAG}")

# Provide an install target iff this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
    install(
        TARGETS ImageIO
        EXPORT ImageIOTargets
        CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
        LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    )
    # Install rule for available public headers
    install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)

    configure_ccp_vendor_config_file(
        TARGET ImageIO
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/ImageIOConfig.cmake"
        DEPENDENCIES ""
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ImageIOConfig.cmake DESTINATION .)

endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_TESTING "Build and run tests. Enabled by default." ON)
    include(CTest)
    if(BUILD_TESTING)
	    add_subdirectory(tests)
    endif()
endif()