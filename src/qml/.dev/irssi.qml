import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

Rectangle {
    id: root
    visible: true
    anchors.top: parent.top
    anchors.left: parent.left
    color: "black"

    width: 600
    height: 800

    property string statusBarBackgroundColor: "#3465a4"
    property string textGreyColor: "#555753"
    property string textDimmedColor: "#d3d7ce"
    property string textNickActiveColor: "#30e0e2"
    property string textNickInactiveColor: "#06989a"

    ListModel {
        id: xxx
        ListElement {
            name: "Wizzup"
            datestr: "11:44"
            message: "I made this thing"
            isHead: true
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44"
            message: "it kinda works"
            isHead: false
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44"
            message: "you should look at itt"
            isHead: false
            isLast: false
        }
        ListElement {
            datestr: "11:32"
            name: "dsc"
            message: "Yeah, that works pretty well!"
            isHead: true
            isLast: false
        }
        ListElement {
            datestr: "10:04"
            name: "Wizzup"
            message: "Agree, but here is a long line, you should word-wrap this! Please fix this ASAP when you have time hehe!"
            status: "sent"
            isHead: true
            isLast: false
        }
        ListElement {
            datestr: "10:02"
            name: "dsc"
            message: "Bla bla bla :D"
            status: "read"
            isHead: true
            isLast: false
        }
        ListElement {
            datestr: "10:02"
            name: "Wizzup"
            message: "Noooooooo"
            status: "read"
            isHead: true
            isLast: false
        }

        ListElement {
            name: "Wizzup"
            datestr: "11:44"
            message: "I made this thing"
            isHead: true
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44"
            message: "it kinda works"
            isHead: false
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44"
            message: "you should look at itt"
            isHead: false
            isLast: false
        }
        ListElement {
            datestr: "11:32"
            name: "dsc"
            message: "Yeah, that works pretty well!"
            isHead: true
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44"
            message: "I made this thing"
            isHead: true
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44"
            message: "it kinda works"
            isHead: false
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44"
            message: "you should look at itt"
            isHead: false
            isLast: false
        }
        ListElement {
            datestr: "11:32"
            name: "dsc"
            message: "Yeah, that works pretty well!"
            isHead: true
            isLast: false
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 26
        color: statusBarBackgroundColor

        Text {
            id: statusBarText
            color: textDimmedColor
            text: "Example chat"
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 12
            anchors.left: parent.left
            font.pointSize: 14
        }
    }

    ListView {
        id: chatListView
        anchors.fill: parent
        anchors.topMargin: 26
        anchors.leftMargin: 4
        anchors.rightMargin: 4

        model: xxx
        delegate: RowLayout {
            id: item
            property int itemHeight: textMessage.implicitHeight
            property bool isSelf: name == "dsc"
            height: itemHeight
            width: parent.width
            spacing: 0

            Text {
                font.pointSize: 14
                color: "white"
                text: datestr
                Layout.rightMargin: 10
                Layout.alignment: Qt.AlignTop
            }

            Text {
                font.pointSize: 12
                color: textGreyColor
                text: "<"
                Layout.alignment: Qt.AlignTop
            }

            Text {
                font.pointSize: 14
                color: "white"
                text: isSelf ? "@" + name : name
                font.bold: isSelf
                Layout.alignment: Qt.AlignTop
            }

            Text {
                font.pointSize: 12
                color: textGreyColor
                text: ">"
                Layout.rightMargin: 6
                Layout.alignment: Qt.AlignTop
            }

            Text {
                id: textMessage
                font.pointSize: 14
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