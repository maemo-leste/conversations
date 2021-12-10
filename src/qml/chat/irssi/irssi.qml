import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

import "../components" as Components

Components.ChatRoot {
    id: root
    color: "black"
    chatList: chatListView

    property string statusBarBackgroundColor: "#3465a4"
    property string textGreyColor: "#555753"
    property string textDimmedColor: "#d3d7ce"
    property string textNickActiveColor: "#30e0e2"
    property string textNickInactiveColor: "#06989a"

    historyPopupBackgroundColor: "#262d31"
    historyPopupTextColor: "white"

    Rectangle {
        z: parent.z + 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 26 * ctx.scaleFactor
        color: statusBarBackgroundColor

        Text {
            id: statusBarText
            color: textDimmedColor
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
            width: parent.width
            spacing: 0

            Text {
                font.pointSize: 14 * ctx.scaleFactor
                font.family: fixedFont
                color: "white"
                text: hourstr
                Layout.rightMargin: 10
                Layout.alignment: Qt.AlignTop
            }

            Text {
                font.pointSize: 12 * ctx.scaleFactor
                font.family: fixedFont
                color: textGreyColor
                text: "<"
                Layout.alignment: Qt.AlignTop
            }

            Text {
                font.pointSize: 14 * ctx.scaleFactor
                font.family: fixedFont
                color: "white"
                text: outgoing ? "@me" : "them"
                font.bold: outgoing
                Layout.alignment: Qt.AlignTop
            }

            Text {
                font.pointSize: 12 * ctx.scaleFactor
                font.family: fixedFont
                color: textGreyColor
                text: ">"
                Layout.rightMargin: 6
                Layout.alignment: Qt.AlignTop
            }

            Text {
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