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
    property string colorHighlight: ctx.inheritSystemTheme ? theme.colors.SelectionColor : "#00a2ff"
    property string colorMutedText: ctx.inheritSystemTheme ? theme.colors.disabledTextColor : "grey"
    property string colorSecondaryText: ctx.inheritSystemTheme ? theme.colors.secondaryTextColor : "grey"

    property string avatarBorderColor: ctx.inheritSystemTheme ? theme.colors.SelectionColor : "#999999"
    property string chatBorderColor: ctx.inheritSystemTheme ? theme.colors.SelectionColor : "#30302f"
    property string nickColor: ctx.inheritSystemTheme ? colorSecondaryText : "#86d5fc"
    property string dividerColor: ctx.inheritSystemTheme ? theme.colors.disabledTextColor : "#2c2c2c"

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
                    }
                }

                Item {
                    // spacer
                    Layout.preferredWidth: 10
                    Layout.minimumHeight: itemHeight
                }

                RowLayout {
                    Layout.fillWidth: true

                    Components.PlainText {
                        id: textNick
                        Layout.alignment: Qt.AlignVCenter
                        Layout.minimumWidth: chatListView.nickWidth
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

                    Components.PlainText {
                        id: textChatMessage
                        visible: isHead
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        Layout.alignment: isHead ? Qt.AlignVCenter : Qt.AlignTop
                        text: message
                        color: root.colorText
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

                        Components.PlainText {
                            Layout.fillWidth: true
                            text: message
                            color: root.colorText
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

                    Components.PlainText {
                        Layout.leftMargin: 12
                        Layout.alignment: Qt.AlignVCenter
                        text: hourstr
                        color: root.colorMutedText
                        font.pointSize: 12 * ctx.scaleFactor
                    }
                }
            }
        }
    }
}
