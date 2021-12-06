import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

import "../components" as Components

Components.ChatRoot {
    id: root
    color: "black"
    chatList: chatListView

    property string avatarBorderColor: "#999999"
    property string chatBorderColor: "#30302f"
    property string nickColor: "#86d5fc"
    property string dividerColor: "#2c2c2c"
    property int itemHeightDefault: 68
    property int itemHeightSmall: 32

    Components.ChatListView {
        id: chatListView
        property int nickWidth: 0
        anchors.fill: parent
        anchors.topMargin: 10
        anchors.leftMargin: 20
        anchors.rightMargin: 20

        onScrollToBottom: root.scrollToBottom();

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

                    Image {
                        visible: isHead
                        width: root.itemHeightDefault - 4
                        height: root.itemHeightDefault - 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        id: imgStatus
                        source: {
                            if(outgoing) return "qrc:///avatar.jpg"
                            else if(name == "Wizzup") return "qrc:///wajer.png"
                            else if(name == "uvos") return "qrc:///uvos.png"
                            return "qrc:///wabbit.png"
                        }
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
                        textFormat: Text.PlainText
                        text: {
                            if(!isHead) return "";
                            if(outgoing) return "<self>";
                            return name
                        }
                        color: nickColor
                        font.pointSize: 18 * ctx.scaleFactor

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
                        textFormat: Text.PlainText
                        text: message
                        color: "white"
                        wrapMode: Text.WordWrap
                        font.pointSize: 14 * ctx.scaleFactor
                        font.bold: name == "_self" ? true : false;
                    }

                    ColumnLayout {
                        id: textChatMessage2
                        spacing: 6
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        visible: !isHead

                        Text {
                            Layout.fillWidth: true
                            textFormat: Text.PlainText
                            text: message
                            color: "white"
                            wrapMode: Text.WordWrap
                            font.pointSize: 14 * ctx.scaleFactor
                            font.bold: name == "_self" ? true : false;
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            visible: !isHead && !isLast
                            color: root.dividerColor
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
                        textFormat: Text.PlainText
                        text: hourstr
                        color: "grey"
                        font.pointSize: 12 * ctx.scaleFactor
                    }
                }
            }
        }
    }

    onFetchHistory: {
        // Prepend new items to the model by calling `getPage()`.
        // temp. disable visibility to 'break' the touch gesture,
        // if we dont the list scrolling bugs out by "jumping"
        chatListView.visible = false;
        var count_results = chatModel.getPage();
        chatListView.positionViewAtIndex(count_results, ListView.Visible)
        chatListView.visible = true;
    }
}
