cmake_minimum_required(VERSION 3.13)
project(conversations)
message(STATUS "Initiating compile using CMake ${CMAKE_VERSION}")

set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_REVISION "0")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}")

# add `cmake` directory as cmake module path
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CONVERSATIONS_DESKTOP_DIR /usr/share/applications/hildon)

configure_file("cmake/config-conversations.h.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/src/config-conversations.h")

set(CMAKE_CXX_STANDARD 14)
include(FindCcache)
include(CheckIncludeFiles)
include(CheckFunctionExists)
#include(CheckSymbolExists)

find_package(PkgConfig REQUIRED)
find_package(GLIB REQUIRED)
pkg_search_module(LIBHILDON hildon-1)
if(DEFINED LIBHILDON_LIBRARIES)
    set(MAEMO ON)
endif()

# rtcom-eventlogger
pkg_check_modules(RTCOM-EVENTLOGGER REQUIRED rtcom-eventlogger)

# Find X11
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_SKIP_RPATH ON)
endif()

find_package(X11 REQUIRED)
message(STATUS "X11_FOUND = ${X11_FOUND}")
message(STATUS "X11_INCLUDE_DIR = ${X11_INCLUDE_DIR}")
message(STATUS "X11_LIBRARIES = ${X11_LIBRARIES}")

include_directories(${X11_INCLUDE_DIR})
link_directories(${X11_LIBRARIES})

# Find pthread
find_package(Threads REQUIRED)

add_subdirectory(src)
add_subdirectory(desktop)
