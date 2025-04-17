set(TARGETS "conversations" "conversations_slim")
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
        "lib/tp/*.h"
        "lib/tp/*.cpp"
        "models/*.h"
        "models/*.cpp"
        "overview/*.cpp"
        "overview/*.h"
        "requests/*.cpp"
        "requests/*.h"
        "lib/libnotify-qt/*.h"
        "lib/libnotify-qt/*.cpp"
        "lib/qtcsv/*.h"
        "lib/qtcsv/*.cpp"
        )

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network DBus Svg Xml Quick QuickWidgets Qml QuickControls2 QuickCompiler)
if(MAEMO)
    find_package(Qt5 REQUIRED COMPONENTS Maemo5)
endif()

find_package(TelepathyQt5 0.9.7 REQUIRED)

qtquick_compiler_add_resources(QUICK_RESOURCES
        qml/components/qml_components.qrc
        qml/chat/chatty/chatty.qrc
        qml/chat/whatsthat/whatsthat.qrc
        qml/chat/irssi/irssi.qrc
)

# conversations
add_executable(conversations ${EXECUTABLE_FLAG} main.cpp
        ${SOURCE_FILES}
        assets.qrc
        ${QUICK_RESOURCES}
)

# conversations_slim
add_executable(conversations_slim ${EXECUTABLE_FLAG} main.cpp
        ${SOURCE_FILES}
        assets.qrc
)

if(DISABLE_QML_DISK_CACHE)
  message(STATUS "QML disk cache: off")
  target_compile_definitions(conversations PRIVATE DISABLE_QML_DISK_CACHE=1)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    target_compile_definitions(conversations PRIVATE DEBUG=1)
    target_compile_definitions(conversations_slim PRIVATE DEBUG=1)
endif()

add_subdirectory(lib/abook/)
add_subdirectory(lib/rtcom/)

foreach(target ${TARGETS})
    set_property(TARGET ${target} PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    target_include_directories(${target} PUBLIC ${Qt5Gui_PRIVATE_INCLUDE_DIRS})
    target_compile_definitions(${target} PRIVATE OSSO_ABOOK_DEBUG=1)
    target_compile_definitions(${target} PRIVATE MAEMO=1)
    target_compile_definitions(${target} PRIVATE TP_QT_ENABLE_LOWLEVEL_API=1)
    target_include_directories(${target} PUBLIC
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/src/lib/
            ${X11_INCLUDE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${Qt5Core_INCLUDE_DIRS}
            ${Qt5Widgets_INCLUDE_DIRS}
            ${Qt5Gui_INCLUDE_DIRS}
            ${Qt5Network_INCLUDE_DIRS}
            ${Qt5DBus_INCLUDE_DIRS}
            ${Qt5Svg_INCLUDE_DIRS}
            ${Qt5Xml_INCLUDE_DIRS}
            ${TELEPATHY_QT5_INCLUDE_DIR}
            ${GLIB_INCLUDE_DIRS}
            ${RTCOM-EVENTLOGGER_INCLUDE_DIRS}
            ${LIBHILDON_INCLUDE_DIRS}
            ${OSSO_ABOOK_INCLUDE_DIRS}
            ${GTK_INCLUDE_DIRS}
            ${LIBEBOOK_INCLUDE_DIRS}
    )

    target_compile_definitions(${target}
            PUBLIC
            ${Qt5Core_DEFINITIONS}
            ${Qt5Widgets_DEFINITIONS}
            ${Qt5Gui_DEFINITIONS}
            ${Qt5DBus_DEFINITIONS}
            ${Qt5Network_DEFINITIONS}
            ${Qt5Svg_DEFINITIONS}
            ${Qt5Xml_DEFINITIONS}
    )

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
            Qt5::Core
            Qt5::Widgets
            Qt5::Gui
            Qt5::DBus
            Qt5::Network
            Qt5::Svg
            Qt5::QSvgIconPlugin
            Qt5::QSvgPlugin
            Qt5::Xml
            Qt5::Maemo5
            Threads::Threads
            abook_qt
            rtcom_qt
            ${TELEPATHY_QT5_LIBRARIES}
            ${LIBHILDON_LIBRARIES}
            ${LIBHILDONMIME_LIBRARIES}
    )

    install(TARGETS ${target} DESTINATION bin)
endforeach()

set(CMAKE_CXX_FLAGS
        ${CMAKE_CXX_FLAGS}
        ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}
        )

target_compile_definitions(conversations PRIVATE QUICK=1)
target_link_libraries(conversations PUBLIC
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Qml
    Qt5::QuickControls2
)

message(STATUS "CONVERSATIONS: ${VERSION} ${CMAKE_BUILD_TYPE}")
if(MAEMO)
    message(STATUS "Maemo build: ON")
else()
    message(STATUS "Maemo build: NO! (not running on maemo? things may not work!)")
endif()
message(STATUS "GLib: ${GLIB_VERSION}")
message(STATUS "tp-qt: ${TELEPATHY_QT5_VERSION}")
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
