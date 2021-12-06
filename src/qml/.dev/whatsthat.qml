import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

Rectangle {
    id: root
    visible: true
    anchors.top: parent.top
    anchors.left: parent.left
    color: "grey"

    width: 600
    height: 800

    property string chatBackgroundSelf: "#056162"
    property string chatBackgroundThem: "#262d31"
    property int itemHeightDefault: 68
    property int itemHeightSmall: 32

    Rectangle {
        color: "grey"
        anchors.fill: parent
    }

    ListModel {
        id: xxx
        ListElement {
            name: "Wizzup"
            datestr: "11:44 pm"
            message: "I made this thing"
            isHead: true
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44 pm"
            message: "it kinda works"
            isHead: false
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44 pm"
            message: "you should look at itt"
            isHead: false
            isLast: false
        }
        ListElement {
            datestr: "11:32 pm"
            name: "dsc"
            message: "Yeah, that works pretty well!"
            isHead: true
            isLast: false
        }
        ListElement {
            datestr: "10:04 pm"
            name: "Wizzup"
            message: "Agree, but here is a long line, you should word-wrap this! Please fix this ASAP when you have time hehe!"
            status: "sent"
            isHead: true
            isLast: false
        }
        ListElement {
            datestr: "10:02 pm"
            name: "dsc"
            message: "Bla bla bla :D"
            status: "read"
            isHead: true
            isLast: false
        }
        ListElement {
            datestr: "10:02 pm"
            name: "wtf"
            message: "Noooooooo"
            status: "read"
            isHead: true
            isLast: false
        }
    }

    ListView {
        id: chatListView
        anchors.fill: parent
        anchors.topMargin: 10
        anchors.leftMargin: 20
        anchors.rightMargin: 20

        model: xxx
        delegate: RowLayout {
            id: item
            property int itemHeight: textColumn.implicitHeight + 12
            property bool isSelf: name == "dsc"
            height: itemHeight + 12
            width: parent.width
            spacing: 0

            Item {
                visible: isSelf
                Layout.fillWidth: true
                Layout.preferredHeight: 32
            }

            Rectangle {
                radius: 4
                clip: true
                color: isSelf ? root.chatBackgroundSelf : root.chatBackgroundThem
                Layout.preferredHeight: itemHeight
                Layout.preferredWidth: {
                    var max_width = item.width / 6 * 5;
                    var meta_width = metaRow.implicitWidth + 32;
                    var text_width = textMessage.implicitWidth + 32;

                    if(meta_width > text_width)
                        if(meta_width < max_width) return meta_width;

                    if(text_width < max_width) return text_width;
                    return max_width;
                }

                ColumnLayout {
                    id: textColumn
                    anchors.fill: parent
                    anchors.margins: 6
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    Layout.fillWidth: true
                    Layout.preferredHeight: itemHeight

                    RowLayout {
                        id: metaRow
                        spacing: 8

                        Item {
                            visible: !isHead
                            Layout.fillWidth: true
                        }

                        Text {
                            visible: isHead
                            font.pointSize: 12
                            color: "lightblue"
                            text: "+44 7878 772185"
                        }

                        Text {
                            font.pointSize: 12
                            color: "#98ac90"
                            text: datestr
                            Layout.rightMargin: 0
                        }

                        Item {
                            visible: isHead
                            Layout.fillWidth: true
                        }

                        Text {
                            visible: isHead
                            font.pointSize: 12
                            color: "#98ac90"
                            text: "~" + name
                            Layout.rightMargin: 0
                        }
                    }

                    Text {
                        id: textMessage
                        color: "white"
                        text: message
                        wrapMode: Text.WordWrap
                        width: parent.width
                        font.pointSize: 14
                        Layout.preferredWidth: parent.width
                    }
                }
            }

            Item {
                visible: !isSelf
                Layout.fillWidth: true
                Layout.preferredHeight: 32
            }
        }
    }
}