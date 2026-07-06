install(FILES Conversations.client DESTINATION ${TELEPATHY_CLIENTS_DIR})

install(FILES org.freedesktop.Telepathy.Client.Conversations.service
        DESTINATION /usr/share/dbus-1/services)
