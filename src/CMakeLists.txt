set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

# the source files
file(GLOB SOURCE_FILES
        "*.h"
        "*.cpp"
        "lib/*.h"
        "lib/*.cpp"
        "models/*.h"
        "models/*.cpp"
        "lib/libnotify-qt/*.h"
        "lib/libnotify-qt/*.cpp"
        "lib/qtcsv/*.h"
        "lib/qtcsv/*.cpp"
        )

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network DBus Svg Quick QuickWidgets Qml QuickControls2 QuickCompiler)
qtquick_compiler_add_resources(RESOURCES
        assets.qrc
        qml/components/qml_components.qrc
        qml/chat/chatty/chatty.qrc
        qml/chat/whatsthat/whatsthat.qrc
        qml/chat/irssi/irssi.qrc
        )

qt5_add_resources(RESOURCES-tiny assets.qrc)

if(MAEMO)
    find_package(Qt5 REQUIRED COMPONENTS Maemo5)
endif()

find_package(TelepathyQt5 0.9.7 REQUIRED)

if(NOT MAEMO)
    qtquick_compiler_add_resources(RESOURCES assets_localdev.qrc)
endif()

set(EXECUTABLE_FLAG)

add_executable(conversations ${EXECUTABLE_FLAG} main.cpp
        ${SOURCE_FILES}
        ${RESOURCES}
        )

add_executable(conversations-tiny ${EXECUTABLE_FLAG} main.cpp
        ${SOURCE_FILES}
        ${RESOURCES-tiny}
        )

set_property(TARGET conversations PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_property(TARGET conversations-tiny PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_include_directories(conversations PUBLIC ${Qt5Gui_PRIVATE_INCLUDE_DIRS})
target_include_directories(conversations-tiny PUBLIC ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

target_compile_definitions(conversations PRIVATE QUICK=1)

if(MAEMO)
    # @TODO: should not be necessary, but definition `Q_WS_MAEMO` is *not* available via CMake
    target_compile_definitions(conversations PRIVATE MAEMO=1)
    target_compile_definitions(conversations-tiny PRIVATE MAEMO=1)
    target_compile_definitions(conversations PRIVATE MAEMO_GTK=1)
    target_compile_definitions(conversations-tiny PRIVATE MAEMO_GTK=1)
endif()

if(RTCOM)
    target_compile_definitions(conversations PRIVATE RTCOM=1)
    target_compile_definitions(conversations-tiny PRIVATE RTCOM=1)
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    target_compile_definitions(conversations PRIVATE DEBUG=1)
    target_compile_definitions(conversations-tiny PRIVATE DEBUG=1)
endif()

set(INCLUDE_DIRS
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
        ${TELEPATHY_QT5_INCLUDE_DIR}
        ${GLIB_INCLUDE_DIRS}
        ${RTCOM-EVENTLOGGER_INCLUDE_DIRS}
        ${LIBHILDON_INCLUDE_DIRS}
        ${OSSO_ABOOK_INCLUDE_DIRS}
        ${GTK_INCLUDE_DIRS})

target_include_directories(conversations PUBLIC ${INCLUDE_DIRS})
target_include_directories(conversations-tiny PUBLIC ${INCLUDE_DIRS})

add_definitions(${QT_DEFINITIONS})
if(NOT "${CMAKE_BUILD_TYPE}" EQUAL "Debug")
    add_definitions(-DQT_NO_DEBUG)
endif()

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS})

set(COMPILE_DEFS
    ${Qt5Core_DEFINITIONS}
    ${Qt5Widgets_DEFINITIONS}
    ${Qt5Gui_DEFINITIONS}
    ${Qt5DBus_DEFINITIONS}
    ${Qt5Network_DEFINITIONS}
    ${Qt5Svg_DEFINITIONS})

set(COMPILE_OPTS
    ${RTCOM-EVENTLOGGER_CFLAGS_OTHER}
    ${LIBHILDON_CFLAGS_OTHER}
    ${OSSO_ABOOK_CFLAGS_OTHER}
    ${GTK_CFLAGS_OTHER}
    -Wno-deprecated-declarations)

target_compile_definitions(conversations PUBLIC ${COMPILE_DEFS})
target_compile_definitions(conversations-tiny PUBLIC ${COMPILE_DEFS})
target_compile_options(conversations PUBLIC ${COMPILE_OPTS})
target_compile_definitions(conversations PRIVATE TP_QT_ENABLE_LOWLEVEL_API=1)
target_compile_definitions(conversations-tiny PRIVATE TP_QT_ENABLE_LOWLEVEL_API=1)

target_link_libraries(conversations PUBLIC
        ${CMAKE_DL_LIBS}
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::DBus
        Qt5::Network
        Qt5::Svg
        Qt5::QSvgIconPlugin
        Qt5::QSvgPlugin
        Qt5::Quick
        Qt5::QuickWidgets
        Qt5::Qml
        Qt5::QuickControls2
        Threads::Threads
        ${TELEPATHY_QT5_LIBRARIES}
        ${GLIB_LIBRARIES}
        ${RTCOM-EVENTLOGGER_LIBRARIES}
        ${OSSO_ABOOK_LIBRARIES}
        ${LIBHILDON_LIBRARIES}
        ${GTK_LIBRARIES}
        -lrtcom-el-plugin-chat
)

target_link_libraries(conversations-tiny PUBLIC
        ${CMAKE_DL_LIBS}
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::DBus
        Qt5::Network
        Qt5::Svg
        Qt5::QSvgIconPlugin
        Qt5::QSvgPlugin
        Threads::Threads
        ${TELEPATHY_QT5_LIBRARIES}
        ${GLIB_LIBRARIES}
        ${RTCOM-EVENTLOGGER_LIBRARIES}
        ${OSSO_ABOOK_LIBRARIES}
        ${LIBHILDON_LIBRARIES}
        ${GTK_LIBRARIES}
        -lrtcom-el-plugin-chat
)

if(MAEMO)
    target_link_libraries(conversations PUBLIC Qt5::Maemo5)
    target_link_libraries(conversations-tiny PUBLIC Qt5::Maemo5)
endif()

if(X11_FOUND)
    target_link_libraries(conversations PUBLIC ${X11_LIBRARIES})
    target_link_libraries(conversations-tiny PUBLIC ${X11_LIBRARIES})
endif()

install(TARGETS conversations DESTINATION bin)
install(TARGETS conversations-tiny DESTINATION bin)

message(STATUS "CONVERSATIONS: ${VERSION}")
if(MAEMO)
    message(STATUS "Maemo build: ON")
else()
    message(STATUS "Maemo build: NO! (not running on maemo? things may not work!)")
endif()
message(STATUS "GLib: ${GLIB_VERSION}")
message(STATUS "tp-qt: ${TELEPATHY_QT5_VERSION}")
if(RTCOM)
    message(STATUS "rtcom-eventlogger: ${RTCOM-EVENTLOGGER_VERSION}")
else()
    message(STATUS "rtcom-eventlogger: off")
endif()
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
