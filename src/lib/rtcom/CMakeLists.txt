add_library(rtcom_qt STATIC
        rtcom_public.h
        rtcom_public.cpp
        rtcom_models.h
        rtcom.h
        rtcom.cpp)

target_include_directories(rtcom_qt PUBLIC
        ${GLIB_INCLUDE_DIRS}
        ${RTCOM-EVENTLOGGER_INCLUDE_DIRS}
        ${GTK_INCLUDE_DIRS}
)

target_link_libraries(rtcom_qt PRIVATE
        ${GLIB_LIBRARIES}
        ${GTK_LIBRARIES}
        PkgConfig::RTCOM-EVENTLOGGER
        PkgConfig::RTCOM-EVENTLOGGER-PLUGINS
)

target_compile_options(rtcom_qt PUBLIC
        ${RTCOM-EVENTLOGGER_CFLAGS_OTHER}
        ${GTK_CFLAGS_OTHER}
)

message(STATUS "rtcom-eventlogger: ${RTCOM-EVENTLOGGER_VERSION}")
message(STATUS "rtcom-eventlogger-plugins: ${RTCOM-EVENTLOGGER-PLUGINS_VERSION}")
