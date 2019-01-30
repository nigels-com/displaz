# Build system for a subset of third party libraries required by displaz.

set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
)

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "-std=c++0x"
        CACHE STRING "Flags used by the compiler during all build types.")
endif()

project(external_libs)

cmake_minimum_required(VERSION 2.8)

set(DOWNLOAD_DIR "${PROJECT_SOURCE_DIR}/downloads"
    CACHE PATH "Location for downloaded source archives")
set(THIRDPARTY_INSTALL_PREFIX "dist"
    CACHE PATH "Location to install third party libraries")

set(CMAKE_INSTALL_PREFIX ${THIRDPARTY_INSTALL_PREFIX})

include(ExternalProject)

ExternalProject_Add(
    ilmbase
    URL https://github.com/openexr/openexr/releases/download/v2.3.0/ilmbase-2.3.0.tar.gz
    URL_MD5 354bf86de3b930ab87ac63619d60c860
    CMAKE_ARGS
        -D CMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
        -D CMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -D CMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
    DOWNLOAD_DIR "${DOWNLOAD_DIR}"
)

ExternalProject_Add(
    laslib
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/LAStools
    CMAKE_ARGS
        -D CMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}
        -D CMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -D CMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
    DOWNLOAD_DIR "${DOWNLOAD_DIR}"
)

