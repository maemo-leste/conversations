cmake_minimum_required(VERSION 3.13)
project(conversations
  VERSION "0.7.56"
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
option(ENABLE_DEBUG_TIMINGS "Write time measurements to /tmp/conversations.timings to e.g. debug boot times." OFF)

set(CONVERSATIONS_DESKTOP_DIR /usr/share/applications/hildon)
set(TELEPATHY_CLIENTS_DIR /usr/share/telepathy/clients)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# generate .h dynamically for version and changelog
configure_file("cmake/config-conversations.h.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/src/config-conversations.h")

file(READ "debian/changelog" DEBIAN_CHANGELOG)
string(REPLACE "\\" "\\\\" DEBIAN_CHANGELOG "${DEBIAN_CHANGELOG}")
string(REPLACE "\"" "\\\"" DEBIAN_CHANGELOG "${DEBIAN_CHANGELOG}")
configure_file("cmake/changelog.h.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/src/changelog.h")

include(CheckIncludeFiles)
include(CheckFunctionExists)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(GLIB REQUIRED)

pkg_check_modules(LIBEBOOK REQUIRED IMPORTED_TARGET libebook-1.2)
pkg_check_modules(TELEPATHY-GLIB REQUIRED IMPORTED_TARGET telepathy-glib)
pkg_check_modules(RTCOM-EVENTLOGGER REQUIRED IMPORTED_TARGET rtcom-eventlogger)
pkg_check_modules(RTCOM-EVENTLOGGER-PLUGINS REQUIRED IMPORTED_TARGET rtcom-eventlogger-plugins)

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
