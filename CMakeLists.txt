set(ENV_EXTRA_CMAKE_MODULES_DIRS $ENV{EXTRA_CMAKE_MODULES_DIRS})
if(ENV_EXTRA_CMAKE_MODULES_DIRS)
  set(EXTRA_CMAKE_MODULES_DIRS ${EXTRA_CMAKE_MODULES_DIRS} ${ENV_EXTRA_CMAKE_MODULES_DIRS})
endif(ENV_EXTRA_CMAKE_MODULES_DIRS)
if(NOT EXTRA_CMAKE_MODULES_DIRS)
  message(FATAL_ERROR
    "Cannot find EXTRA_CMAKE_MODULES_DIRS.
1. Please clone it from https://ssl.izenesoft.cn/cmake.
2. Then set EXTRA_CMAKE_MODULES_DIRS through environment variable

       export EXTRA_CMAKE_MODULES_DIRS=/path/to/cmake

   You can put it in your .bashrc so it is loaded automatically.

   or cmake options

       cmake  -DEXTRA_CMAKE_MODULES_DIRS=/path/to/cmake (other options)
")
endif(NOT EXTRA_CMAKE_MODULES_DIRS)
set(CMAKE_MODULE_PATH ${EXTRA_CMAKE_MODULES_DIRS})

project(thegrocer)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -Wextra")

FIND_PACKAGE(izenecma REQUIRED)
find_package(Boost COMPONENTS system thread program_options regex filesystem REQUIRED)

MESSAGE("izenecma root ${izenecma_ROOT_DIR}")
MESSAGE("izenecma include dir ${izenecma_INCLUDE_DIR}")

#include(cmake/locate_library.cmake)
#locate_library(URIPARSER "Uri.h" "uriparser" "uriparser")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")

include_directories(
    #    ${URIPARSER_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${izenecma_INCLUDE_DIR}
    #    ${blackhole_INCLUDE_DIR}
)

link_directories(
    #    ${URIPARSER_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS}
    #    ${izenecma_LIBRARY_DIRS}
)

add_subdirectory(source)

