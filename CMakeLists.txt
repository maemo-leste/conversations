cmake_minimum_required(VERSION 3.13)
project(conversations
  VERSION "0.7.17"
  DESCRIPTION "conversations"
)
message(STATUS "Initiating compile using CMake ${CMAKE_VERSION}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Default build type: Debug" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_SKIP_RPATH ON)
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")

option(DISABLE_QML_DISK_CACHE "Disables the disk cache and forces QML/JS re-compilation. Should be ON during QML development." OFF)

set(CONVERSATIONS_DESKTOP_DIR /usr/share/applications/hildon)
set(TELEPATHY_CLIENTS_DIR /usr/share/telepathy/clients)
set(THREADS_PREFER_PTHREAD_FLAG ON)

configure_file("cmake/config-conversations.h.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/src/config-conversations.h")

include(FindCcache)
include(CheckIncludeFiles)
include(CheckFunctionExists)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(GLIB REQUIRED)
pkg_check_modules(RTCOM-EVENTLOGGER REQUIRED rtcom-eventlogger)
if(DEFINED RTCOM-EVENTLOGGER_LIBRARY_DIRS)
    set(RTCOM ON)
endif()

pkg_check_modules(LIBHILDON hildon-1)
if(DEFINED LIBHILDON_LIBRARIES)
    set(MAEMO ON)
endif()

pkg_check_modules(GTK REQUIRED gtk+-2.0)
if(DEFINED GTK_LIBRARIES)
    set(GTK ON)
endif()

pkg_check_modules(OSSO_ABOOK REQUIRED libosso-abook-1.0)
if(DEFINED OSSO_ABOOK_FOUND)
    set(OSSO_ABOOK ON)
endif()

#ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

find_package(X11 REQUIRED)
include_directories(${X11_INCLUDE_DIR})
link_directories(${X11_LIBRARIES})

add_subdirectory(src)
add_subdirectory(desktop)
add_subdirectory(telepathy)
