import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "black"
    width: 900
    height: 800
    property string avatarBorderColor: "#999999"
    property string chatBorderColor: "#30302f"
    property string nickColor: "#86d5fc"
    property int itemHeightDefault: 68
    property int itemHeightSmall: 32

    ListModel {
        id: xxx
        ListElement {
            name: "Wizzup"
            datestr: "11:44 pm"
            message: "some 1 line message"
            isHead: true
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44 pm"
            message: "rip"
            isHead: false
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44 pm"
            message: "why this no workie workie"
            isHead: false
            isLast: false
        }
        ListElement {
            name: "Wizzup"
            datestr: "11:44 pm"
            message: "somewhat of a long line here"
            isHead: false
            isLast: true
        }
        ListElement {
            datestr: "11:32 pm"
            name: "dsc"
            message: "Yeah, still drinking!"
            isHead: true
            isLast: false
        }
        ListElement {
            datestr: "10:04 pm"
            name: "Wizzup"
            message: "Agree, but we don't have enough info to implement more than what was already implemented, like pattern fills etc. But yeah, in theory someone could do it. Not me, at least not soon. Copy acceleration is there, I expect glmark-results. Let see what will be in landscape. Are you still drinking?"
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
        property int nickWidth: 0
        anchors.fill: parent
        anchors.topMargin: 10
        anchors.leftMargin: 20
        anchors.rightMargin: 20

        model: xxx
        delegate: Rectangle {
            property int itemHeight: isHead ? root.itemHeightDefault : root.itemHeightSmall;
            height: {
                var implicit = isHead ? textChatMessage.implicitHeight : textChatMessage2.implicitHeight;
                var dynamic_height = implicit + 4;
                if(dynamic_height > itemHeight) return dynamic_height;
                return itemHeight;
            }
            width: parent.width
            color: "black"

            RowLayout {
                spacing: 0
                width: parent.width

                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    color: "transparent"
                    Layout.preferredWidth: itemHeightDefault
                    Layout.minimumHeight: itemHeight

                    Rectangle {
                        visible: isHead
                        width: root.itemHeightDefault - 4
                        height: root.itemHeightDefault - 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        id: imgStatus
                        smooth: true
                    }
                }

                Item {
                    // spacer
                    Layout.preferredWidth: 10
                    Layout.minimumHeight: itemHeight
                }


                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        id: textNick
                        Layout.alignment: Qt.AlignVCenter
                        Layout.minimumWidth: chatListView.nickWidth
                        text: {
                            if(!isHead) return "";
                            if(name == "_self") return "d4irc";
                            return name
                        }
                        color: nickColor
                        font.pointSize: 18

                        Component.onCompleted: {
                            if(implicitWidth > chatListView.nickWidth)
                                chatListView.nickWidth = implicitWidth
                        }
                    }

                    Text {
                        id: textChatMessage
                        visible: isHead
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.alignment: isHead ? Qt.AlignVCenter : Qt.AlignTop
                        text: message
                        color: "white"
                        wrapMode: Text.WordWrap
                        font.pointSize: 14
                        font.bold: name == "dsc" ? true : false;
                    }

                    ColumnLayout {
                        id: textChatMessage2
                        spacing: 6
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        visible: !isHead

                        Text {
                            //Layout.preferredWidth: parent.width
                            Layout.fillWidth: true
                            text: message
                            color: "white"
                            wrapMode: Text.WordWrap
                            font.pointSize: 14
                            font.bold: name == "dsc" ? true : false;
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            visible: !isHead && !isLast
                            color: "grey"
                        }

                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }
                    }

                    Item {
                        Layout.minimumHeight: itemHeight
                        Layout.fillWidth: true
                    }

                    Text {
                        Layout.leftMargin: 12
                        Layout.alignment: Qt.AlignVCenter
                        text: "hourstr"
                        color: "grey"
                        font.pointSize: 12
                    }
                }
            }
        }
    }

}
