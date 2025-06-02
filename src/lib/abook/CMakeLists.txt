add_library(abook_qt STATIC
    abook_public.h
    abook_public.cpp
    abook_roster.h
    abook_contact.h
    abook.h
    abook.cpp)

target_include_directories(abook_qt PUBLIC
    ${GLIB_INCLUDE_DIRS}
    ${RTCOM-EVENTLOGGER_INCLUDE_DIRS}
    ${OSSO_ABOOK_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
    ${LIBEBOOK_INCLUDE_DIRS}
)

target_link_libraries(abook_qt PRIVATE
    ${GLIB_LIBRARIES}
    ${OSSO_ABOOK_LIBRARIES}
    PkgConfig::LIBEBOOK
    PkgConfig::TELEPATHY-GLIB
    logger_std
)

target_compile_options(abook_qt PUBLIC
    ${RTCOM-EVENTLOGGER_CFLAGS_OTHER}
    ${OSSO_ABOOK_CFLAGS_OTHER}
    ${GTK_CFLAGS_OTHER}
)
