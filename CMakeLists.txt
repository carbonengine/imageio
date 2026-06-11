# Copyright © 2021 CCP ehf.

cmake_minimum_required(VERSION 3.16)

project(carbon-imageio VERSION 1.0.0)

include(cmake/CcpTargetConfigurations.cmake)
include(cmake/CcpDocsGenerator.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)
include(cmake/CcpBuildConfigurations.cmake)

ccp_add_library(CcpImageIO STATIC
        CcpMetadata.cpp
        HostBitmap.cpp
        ImageIOResult.cpp
        ImageUtility.cpp
		NanoVdbSupport.cpp
        PsdHandler.cpp
        StdAfx.cpp
        Tr2BmpHandler.cpp
        Tr2DdsHandler.cpp
        Tr2ImageHandler.cpp
        Tr2JpgHandler.cpp
        Tr2PngHandler.cpp
        Tr2TgaHandler.cpp
		VtaHandler.cpp
)

set(PUBLIC_HEADER_FILES
    include/BitmapDimensions.h
    include/CubemapFace.h
    include/CcpMetadata.h
    include/HostBitmap.h
    include/ImageIOResult.h
    include/ImageUtility.h
    include/NanoVdbSupport.h
    include/PixelFormat.h
    include/PsdHandler.h
    include/TextureType.h
    include/Tr2BmpHandler.h
    include/Tr2DdsHandler.h
    include/Tr2ImageHandler.h
    include/Tr2JpgHandler.h
    include/Tr2PngHandler.h
    include/Tr2TgaHandler.h
    include/VtaHandler.h
)

target_sources(CcpImageIO PRIVATE ${PUBLIC_HEADER_FILES})

target_precompile_headers(CcpImageIO PRIVATE StdAfx.h)
set_target_properties(CcpImageIO PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME CcpImageIO)

target_precompile_headers(CcpImageIO PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)

find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(PNG CONFIG REQUIRED)
find_package(zlib REQUIRED)
find_package(carbon-core CONFIG REQUIRED)
find_package(OpenVDB REQUIRED)

target_include_directories(CcpImageIO
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(CcpImageIO PUBLIC
        CcpCore
        $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
        PNG::PNG
        ZLIB::ZLIB
        OpenVDB::openvdb
)

target_compile_options(CcpImageIO PRIVATE "${MATH_OPTIMIZE_FLAG}")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(BUILD_TESTING "Build and run tests. Enabled by default." ON)
    include(CTest)
    if(BUILD_TESTING)
	    add_subdirectory(tests)
    endif()
endif()

# Provide an install target iff this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

 option(INSTALL_TO_MONOLITH "Install this package to a perforce branch vendor folder - provided for backwards compatability" OFF)
    if (INSTALL_TO_MONOLITH)

        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        install(
            TARGETS CcpImageIO
            EXPORT CcpImageIOTargets
            CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
            ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
            RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        )
        # Install rule for available public headers
        install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)

        configure_ccp_vendor_config_file(
            TARGET CcpImageIO
            DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/carbon-imageioConfig.cmake"
            DEPENDENCIES JPEG PNG ZLIB NanoVDB
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/carbon-imageioConfig.cmake DESTINATION .)
    else ()

        # Install rule to ensure that our runtime and linker files are in the vcpkg structure
        install(
                TARGETS CcpImageIO
                EXPORT CcpImageIOTarget
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin
        )

         # Install rule for available public headers
        install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)

        install(EXPORT CcpImageIOTarget
                DESTINATION share/carbon-imageio
                FILE carbon-imageioTargets.cmake
        )

        include(CMakePackageConfigHelpers)

        configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/carbon-imageioConfig.cmake.in 
                                      ${CMAKE_CURRENT_BINARY_DIR}/carbon-imageioConfig.cmake
                                      INSTALL_DESTINATION share/carbon-imageio)

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/carbon-imageioConfig.cmake
                DESTINATION share/carbon-imageio)

    endif()

endif()