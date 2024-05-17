cmake_minimum_required(VERSION 3.13)
project(conversations)
message(STATUS "Initiating compile using CMake ${CMAKE_VERSION}")

set(VERSION_MAJOR "0")
set(VERSION_MINOR "5")
set(VERSION_REVISION "7")
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}")

set(CMAKE_CXX_STANDARD 20)
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")
set(CONVERSATIONS_DESKTOP_DIR /usr/share/applications/hildon)
set(TELEPATHY_CLIENTS_DIR /usr/share/telepathy/clients)
set(THREADS_PREFER_PTHREAD_FLAG ON)
if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_SKIP_RPATH ON)
endif()

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