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

    Rectangle {
        z: parent.z + 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 26 * ctx.scaleFactor
        color: statusBarBackgroundColor

        Components.PlainText {
            id: statusBarText
            color: statusBarTextColor
            text: chatModel.remote_uid
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 12
            anchors.left: parent.left
            font.pointSize: 14 * ctx.scaleFactor
        }
    }

    Components.ChatListView {
        id: chatListView

        anchors.fill: parent
        anchors.topMargin: 26 * ctx.scaleFactor
        anchors.leftMargin: 4
        anchors.rightMargin: 4

        onScrollToBottom: root.scrollToBottom();

        delegate: RowLayout {
            id: item
            property int itemHeight: textMessage.implicitHeight
            height: itemHeight
            width: parent !== null ? parent.width : 0
            spacing: 0

            Components.PlainText {
                font.pointSize: 14 * ctx.scaleFactor
                font.family: fixedFont
                color: root.colorActiveTextColor
                text: hourstr
                Layout.rightMargin: 10
                Layout.alignment: Qt.AlignTop
            }

            Components.PlainText {
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
                color: "white"
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: itemHeight
            }
        }
    }
}
