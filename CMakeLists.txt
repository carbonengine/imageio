cmake_minimum_required(VERSION 3.16)

project(ImageIO VERSION 1.0.0)

include(cmake/CcpTargetConfigurations.cmake)
include(cmake/CcpDocsGenerator.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)
include(cmake/CcpBuildConfigurations.cmake)

ccp_add_library(ImageIO STATIC
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

target_sources(ImageIO PRIVATE ${PUBLIC_HEADER_FILES})

target_precompile_headers(ImageIO PRIVATE StdAfx.h)
set_target_properties(ImageIO PROPERTIES POSITION_INDEPENDENT_CODE ON EXPORT_NAME ImageIO)

target_precompile_headers(ImageIO PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h)

find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(zlib-ng REQUIRED)
find_package(carbon-core CONFIG REQUIRED)
find_package(OpenVDB REQUIRED)

target_include_directories(ImageIO
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(ImageIO PUBLIC CcpCore JPEG::JPEG PNG::PNG zlib-ng::zlib OpenVDB::openvdb)

target_compile_options(ImageIO PRIVATE "${MATH_OPTIMIZE_FLAG}")

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
        include(cmake/CcpMonolithVendorConfig.cmake)
        include(cmake/CcpPackageConfigHelpers.cmake)

        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        install(
            TARGETS ImageIO
            EXPORT ImageIOTargets
            CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            LIBRARY DESTINATION ${CCP_VENDOR_LIB_PATH}
            ARCHIVE DESTINATION ${CCP_VENDOR_LIB_PATH}
            RUNTIME DESTINATION ${CCP_VENDOR_BIN_PATH}
        )
        # Install rule for available public headers
        install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)

        configure_ccp_vendor_config_file(
            TARGET ImageIO
            DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/ImageIOConfig.cmake"
            DEPENDENCIES JPEG PNG ZLIB NanoVDB
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ImageIOConfig.cmake DESTINATION .)
    else ()

        # Install rule for available public headers
        install(
                TARGETS ImageIO
                EXPORT ImageIOTarget
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                DESTINATION ${CMAKE_INSTALL_PREFIX}/share/carbon-imageio
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin
        )

        install(EXPORT ImageIOTarget
                DESTINATION share/carbon-imageio
                FILE carbon-imageioConfig.cmake
        )

    endif()

endif()