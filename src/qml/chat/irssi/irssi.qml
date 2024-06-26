import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

import "../components" as Components

Components.ChatRoot {
    id: root
    color: colorBackground
    chatList: chatListView

    property string colorBackground: ctx.inheritSystemTheme ? theme.colors.defaultBackgroundColor : "black"
    property string colorText: ctx.inheritSystemTheme ? theme.colors.defaultTextColor : "white"
    property string colorMutedText: ctx.inheritSystemTheme ? theme.colors.disabledTextColor : "grey"
    property string colorSecondaryText: ctx.inheritSystemTheme ? theme.colors.secondaryTextColor : "grey"
    property string colorActiveTextColor: ctx.inheritSystemTheme ? theme.colors.activeTextColor : "white"
    property string statusBarBackgroundColor: ctx.inheritSystemTheme ? theme.colors.reversedBackgroundColor : "#00a2ff"
    property string statusBarTextColor: ctx.inheritSystemTheme ? theme.colors.reversedTextColor : textDimmedColor
    property string textGreyColor: ctx.inheritSystemTheme ? theme.colors.secondaryTextColor : "#555753"
    property string textDimmedColor: ctx.inheritSystemTheme ? theme.colors.disabledTextColor : "#d3d7ce"
    property string textNickActiveColor: ctx.inheritSystemTheme ? theme.colors.defaultTextColor : "#30e0e2"
    property string textNickInactiveColor: ctx.inheritSystemTheme ? theme.colors.secondaryTextColor : "#06989a"

    historyPopupBackgroundColor: ctx.inheritSystemTheme ? theme.colors.statusBarBackgroundColor : "262d31"
    historyPopupTextColor: ctx.inheritSystemTheme ? theme.colors.statusBarTextColor : "white"

    Components.ChatListView {
        id: chatListView

        anchors.fill: parent
        anchors.leftMargin: 4
        anchors.rightMargin: 4

        onScrollToBottom: root.scrollToBottom();

        delegate: RowLayout {
            id: item
            property int itemHeight: textMessage.implicitHeight
            // handy snippet to determine if this current delegate is in view, in case
            // we need it in the future
            // property int yoff: Math.round(item.y - chatListView.contentY)
            // property bool isFullyVisible: {
            //     let _visible = yoff > chatListView.y && yoff + height < chatListView.y + chatListView.height;
            //     return _visible;
            // }
            height: itemHeight
            width: parent !== null ? parent.width : 0
            spacing: 3

            Components.PlainText {
                font.pointSize: 14 * ctx.scaleFactor
                font.family: fixedFont
                color: root.colorActiveTextColor
                text: hourstr
                Layout.rightMargin: 10
                Layout.alignment: Qt.AlignTop
            }

            Components.PlainText {
                visible: chat_event
                font.pointSize: 12 * ctx.scaleFactor
                font.family: fixedFont
                color: textGreyColor
                text: "<"
                Layout.alignment: Qt.AlignTop
            }

            Components.PlainText {
                font.pointSize: 14 * ctx.scaleFactor
                font.family: fixedFont
                color: "white"
                text: outgoing ? "@me" : name
                font.bold: outgoing
                Layout.alignment: Qt.AlignTop
            }

            Components.PlainText {
                visible: chat_event
                font.pointSize: 12 * ctx.scaleFactor
                font.family: fixedFont
                color: textGreyColor
                text: ">"
                Layout.rightMargin: 6
                Layout.alignment: Qt.AlignTop
            }

            Components.PlainText {
                id: textMessage
                font.family: fixedFont
                font.pointSize: 14 * ctx.scaleFactor
                text: message
                color: !chat_event ? textGreyColor : "white"
                Layout.fillWidth: true
                wrapMode: hardWordWrap ? Text.WrapAnywhere : Text.WordWrap
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
        }
    }
}
