cmake_minimum_required( VERSION 3.11 )
project( veeam-signature
         VERSION 1.0
         DESCRIPTION  "Command line signature generator"
         HOMEPAGE_URL "https://github.com/fslobanov/veeam-signature"
         LANGUAGES "CXX" )

if( NOT UNIX )
	message( SEND_ERROR "Only linux is supported" )
endif()

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/artifacts" CACHE STRING "runtime output directory" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE STRING "release runtime output directory" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} CACHE STRING "debug runtime output directory" )

message( STATUS "veeam-signature: searching for static Boost libraries >= 1.66 .." )

set( Boost_USE_STATIC_LIBS ON CACHE BOOL "Enable Boost static library usage" )
set( Boost_USE_MULTITHREADED ON CACHE BOOL "Enable Boost multithreading support usage" )

find_package(
		Boost 1.66 REQUIRED

		COMPONENTS

		system
		thread
		regex
		filesystem
		program_options
)

message( STATUS "veeam-signature: Boost found: ${Boost_LIB_VERSION}, ${Boost_INCLUDE_DIRS}, ${Boost_LIBRARIES}" )

set( SPDLOG_FMT_EXTERNAL_HO OFF CACHE BOOL "Spdlog uses external header-only fmt" )
set( SPDLOG_BUILD_TESTS OFF CACHE BOOL "Do not build spdlog tests" )
set( SPDLOG_BUILD_TESTS_HO OFF CACHE BOOL "Do not build spdlog-header-only tests" )
set( SPDLOG_BUILD_BENCH OFF CACHE BOOL "Do not build spdlog benchmarks" )
set( SPDLOG_BUILD_WARNINGS OFF CACHE BOOL "Do not enable spdlog warnings" )
set( SPDLOG_INSTALL OFF CACHE BOOL "Do not install spdlog" )
add_subdirectory( spdlog-1.8.2 )

add_subdirectory( src )