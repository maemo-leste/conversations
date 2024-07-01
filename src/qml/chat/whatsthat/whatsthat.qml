import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

import "../components" as Components
import "."

Components.ChatRoot {
    id: root
    chatList: chatListView
    color: colorBackground

    property string colorBackground: ctx.inheritSystemTheme ? theme.colors.defaultBackgroundColor : "black"
    property string chatBackgroundSelf: ctx.inheritSystemTheme ? theme.colors.notificationBackgroundColor : "#056162"
    property string chatBackgroundThem: ctx.inheritSystemTheme ? theme.colors.contentSelectionColor : "#363e42"

    property string colorTextSelf: ctx.inheritSystemTheme ? theme.colors.defaultTextColor : "white"
    property string colorTextThem: ctx.inheritSystemTheme ? theme.colors.paintedTextColor : "white"

    property string colorHighlight: ctx.inheritSystemTheme ? theme.colors.contentSelectionColor : "white"
    property string colorReversedTextColor: ctx.inheritSystemTheme ? theme.colors.reversedTextColor : "lightblue"
    property string colorReversedSecondaryTextColor: ctx.inheritSystemTheme ? theme.colors.reversedSecondaryTextColor : "lightblue"
    property string colorDate: ctx.inheritSystemTheme ? theme.colors.reversedPaintedTextColor : "#98ac90"

    property int itemHeightDefault: 68
    property int itemHeightSmall: 32

    historyPopupBackgroundColor: "#262d31"
    historyPopupTextColor: "white"

    signal showMessageContextMenu(int event_id, var point)

    Image {
        // background
        visible: !ctx.inheritSystemTheme
        source: "qrc:/whatsthat/bg.png"
        anchors.fill: parent
        fillMode: Image.Tile
    }

    Components.ChatListView {
        id: chatListView

        anchors.fill: parent
        anchors.topMargin: 10
        anchors.leftMargin: 32
        anchors.rightMargin: 32

        onScrollToBottom: root.scrollToBottom();

        delegate: MessageDelegate {
            highlight: root.highlightEventId == event_id
            onShowMessageContextMenu: root.showMessageContextMenu(event_id, point);
        }
    }
}
