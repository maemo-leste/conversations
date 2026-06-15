# launcher
add_executable(conversations launcher.c lib/utils_c.c lib/utils_c.h)
target_include_directories(conversations PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(conversations PROPERTIES
        LINKER_LANGUAGE C
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
check_include_file("sys/prctl.h" HAVE_SYS_PRCTL_H)
if(HAVE_SYS_PRCTL_H)
    target_compile_definitions(conversations PRIVATE HAVE_SYS_PRCTL_H=1)
endif()
target_compile_definitions(conversations PRIVATE INSTALL_PREFIX_QUOTED=\"${CMAKE_INSTALL_PREFIX}\")
install(TARGETS conversations DESTINATION bin)

# conversations
set(TARGETS "conversations_qml" "conversations_slim")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(EXECUTABLE_FLAG)

add_definitions(${QT_DEFINITIONS})
if(NOT "${CMAKE_BUILD_TYPE}" MATCHES "Debug")
    add_definitions(-DQT_NO_DEBUG)
endif()

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

# the source files
file(GLOB SOURCE_FILES
        "*.h"
        "*.cpp"
        "lib/*.h"
        "lib/*.cpp"
        "lib/utils_c.c"
        "lib/tp/*.h"
        "lib/tp/*.cpp"
        "contacts/*.h"
        "contacts/*.cpp"
        "models/*.h"
        "models/*.cpp"
        "overview/*.cpp"
        "overview/*.h"
        "requests/*.cpp"
        "requests/*.h"
        "lib/libnotify-qt/*.h"
        "lib/libnotify-qt/*.cpp"
        )

find_package(qtcsv REQUIRED)  # https://git.maemo.org/leste-upstream-forks/qt6csv
find_package(qt6maemo REQUIRED)  # https://git.maemo.org/leste/qt6-maemo-widgets
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network DBus Svg Xml Quick QuickWidgets Qml QuickControls2 Core5Compat)

find_package(TelepathyQt6 REQUIRED COMPONENTS Core Service)

# conversations
add_executable(conversations_qml ${EXECUTABLE_FLAG} main.cpp
        ${SOURCE_FILES}
        assets.qrc
        qml/components/qml_components.qrc
        qml/chat/chatty/chatty.qrc
        qml/chat/whatsthat/whatsthat.qrc
        qml/chat/irssi/irssi.qrc
)

# conversations_slim
add_executable(conversations_slim ${EXECUTABLE_FLAG} main.cpp
        ${SOURCE_FILES}
        assets.qrc
)

if(DISABLE_QML_DISK_CACHE)
  message(STATUS "QML disk cache: off")
  target_compile_definitions(conversations_qml PRIVATE DISABLE_QML_DISK_CACHE=1)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    target_compile_definitions(conversations_qml PRIVATE DEBUG=1)
    target_compile_definitions(conversations_slim PRIVATE DEBUG=1)
endif()

add_subdirectory(lib/logger_std/)
add_subdirectory(lib/abook/)
add_subdirectory(lib/rtcom/)

foreach(target ${TARGETS})
    set_property(TARGET ${target} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    target_include_directories(${target} PUBLIC ${Qt6Gui_PRIVATE_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE OSSO_ABOOK_DEBUG=1)
    target_compile_definitions(${target} PRIVATE MAEMO=1)
    target_compile_definitions(${target} PRIVATE TP_QT_ENABLE_LOWLEVEL_API=1)
    target_compile_definitions(${target} PRIVATE INSTALL_PREFIX_QUOTED=\"${CMAKE_INSTALL_PREFIX}\")
    target_include_directories(${target} PUBLIC
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/lib/
            ${X11_INCLUDE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${Qt6Core_INCLUDE_DIRS}
            ${Qt6Widgets_INCLUDE_DIRS}
            ${Qt6Gui_INCLUDE_DIRS}
            ${Qt6Network_INCLUDE_DIRS}
            ${Qt6DBus_INCLUDE_DIRS}
            ${Qt6Svg_INCLUDE_DIRS}
            ${Qt6Xml_INCLUDE_DIRS}
            ${GLIB_INCLUDE_DIRS}
            ${RTCOM-EVENTLOGGER_INCLUDE_DIRS}
            ${LIBHILDON_INCLUDE_DIRS}
            ${OSSO_ABOOK_INCLUDE_DIRS}
            ${GTK_INCLUDE_DIRS}
            ${LIBEBOOK_INCLUDE_DIRS}
    )

    target_compile_definitions(${target}
            PUBLIC
            ${Qt6Core_DEFINITIONS}
            ${Qt6Widgets_DEFINITIONS}
            ${Qt6Gui_DEFINITIONS}
            ${Qt6DBus_DEFINITIONS}
            ${Qt6Network_DEFINITIONS}
            ${Qt6Svg_DEFINITIONS}
            ${Qt6Xml_DEFINITIONS}
    )

    if(ENABLE_DEBUG_TIMINGS)
        target_compile_definitions(${target}
            PUBLIC
            ENABLE_DEBUG_TIMINGS=1
        )
    endif()

    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")  # amd64 is exclusively used @ VM
        target_compile_definitions(${target} PRIVATE LESTE_VM=1)
        message(STATUS "LESTE_VM=1")
    endif()

    target_compile_options(${target}
            PUBLIC
            ${RTCOM-EVENTLOGGER_CFLAGS_OTHER}
            ${LIBHILDON_CFLAGS_OTHER}
            ${OSSO_ABOOK_CFLAGS_OTHER}
            ${GTK_CFLAGS_OTHER}
            -Wno-deprecated-declarations
    )

    target_include_directories(${target} PRIVATE lib/abook/)
    target_include_directories(${target} PRIVATE lib/rtcom/)

    target_link_libraries(${target} PUBLIC
            ${CMAKE_DL_LIBS}
            ${X11_LIBRARIES}
            Qt6::Core
            Qt6::Widgets
            Qt6::Gui
            Qt6::DBus
            Qt6::Network
            Qt6::Svg
            Qt6::Xml
            Qt6::Core5Compat
            Threads::Threads
            logger_std
            abook_qt
            rtcom_qt
            qtcsv
            TelepathyQt6::Core
            TelepathyQt6::Service
            ${LIBHILDON_LIBRARIES}
            ${LIBHILDONMIME_LIBRARIES}
            qt6maemo::qt6maemo
    )

    install(TARGETS ${target} DESTINATION bin)
endforeach()

target_compile_definitions(conversations_qml PRIVATE QUICK=1)
target_link_libraries(conversations_qml PUBLIC
    Qt6::Quick
    Qt6::QuickWidgets
    Qt6::Qml
    Qt6::QuickControls2
)

message(STATUS "CONVERSATIONS: ${VERSION} ${CMAKE_BUILD_TYPE}")
if(MAEMO)
    message(STATUS "Maemo build: ON")
else()
    message(STATUS "Maemo build: NO! (not running on maemo? things may not work!)")
endif()
message(STATUS "GLib: ${GLIB_VERSION}")
message(STATUS "tp-qt: ${TelepathyQt6_VERSION}")
if(OSSO_ABOOK)
    message(STATUS "osso-abook: ${OSSO_ABOOK_VERSION}")
else()
    message(STATUS "osso-abook: off")
endif()
if(GTK)
    message(STATUS "gtk: ${GTK_VERSION}")
else()
    message(STATUS "gtk: off")
endif()
if(ENABLE_DEBUG_TIMINGS)
    message(STATUS "debug timings: on")
else()
    message(STATUS "debug timings: off")
endif()