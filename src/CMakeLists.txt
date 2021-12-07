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
        )

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network Svg Xml Quick QuickWidgets Qml QuickControls2 QuickCompiler)
if(MAEMO)
    find_package(Qt5 REQUIRED COMPONENTS Maemo5)
endif()

qtquick_compiler_add_resources(RESOURCES
        assets.qrc
        qml/components/qml_components.qrc
        qml/chat/chatty/chatty.qrc
        qml/chat/whatsthat/whatsthat.qrc
        qml/chat/irssi/irssi.qrc
        )

if(NOT MAEMO)
    qtquick_compiler_add_resources(RESOURCES assets_localdev.qrc)
endif()

set(EXECUTABLE_FLAG)

add_executable(conversations ${EXECUTABLE_FLAG} main.cpp
        ${SOURCE_FILES}
        ${RESOURCES}
        )

set_property(TARGET conversations PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_include_directories(conversations PUBLIC ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

if(MAEMO)
    # @TODO: should not be necessary, but definition `Q_WS_MAEMO` is *not* available via CMake
    target_compile_definitions(conversations PRIVATE MAEMO=1)
endif()

target_include_directories(conversations PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${X11_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Widgets_INCLUDE_DIRS}
        ${Qt5Gui_INCLUDE_DIRS}
        ${Qt5Network_INCLUDE_DIRS}
        ${Qt5Svg_INCLUDE_DIRS}
        ${Qt5Xml_INCLUDE_DIRS}
        ${GLib_INCLUDE_DIRS}
        ${RTCOM-EVENTLOGGER_INCLUDE_DIRS}
        )

add_definitions(${QT_DEFINITIONS})
if(NOT "${CMAKE_BUILD_TYPE}" EQUAL "Debug")
    add_definitions(-DQT_NO_DEBUG)
endif()

target_compile_definitions(conversations PRIVATE HAS_QML=1)

target_compile_definitions(conversations
        PUBLIC
        ${Qt5Core_DEFINITIONS}
        ${Qt5Widgets_DEFINITIONS}
        ${Qt5Gui_DEFINITIONS}
        ${Qt5Network_DEFINITIONS}
        ${Qt5Svg_DEFINITIONS}
        ${Qt5Xml_DEFINITIONS}
        ${GLib_DEFINITIONS}
        )

set(CMAKE_CXX_FLAGS
        ${CMAKE_CXX_FLAGS}
        ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS})

target_link_libraries(conversations PUBLIC
        ${CMAKE_DL_LIBS}
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::Network
        Qt5::Svg
        Qt5::QSvgIconPlugin
        Qt5::QSvgPlugin
        Qt5::Xml
        Qt5::Quick
        Qt5::QuickWidgets
        Qt5::Qml
        Qt5::QuickControls2
        Threads::Threads
        ${GLib_LIBRARY}
        ${RTCOM-EVENTLOGGER_LIBRARIES}
)

if(MAEMO)
    target_link_libraries(conversations PUBLIC Qt5::Maemo5)
endif()

if(X11_FOUND)
    target_link_libraries(conversations PUBLIC ${X11_LIBRARIES})
endif()

install(TARGETS conversations
        DESTINATION bin
        )


message(STATUS "=============================================")
message(STATUS "VERSION_MAJOR: ${VERSION_MAJOR}")
message(STATUS "VERSION_MINOR: ${VERSION_MINOR}")
message(STATUS "VERSION_REVISION: ${VERSION_REVISION}")
message(STATUS "VERSION: ${VERSION}")
message(STATUS "This build is for Maemo: ${MAEMO}")
message(STATUS "GLib: ${GLib_VERSION}")
message(STATUS "rtcom-eventlogger: ${RTCOM-EVENTLOGGER_VERSION}")
message(STATUS "=============================================")
