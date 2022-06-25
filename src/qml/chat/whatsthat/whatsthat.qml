import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

import "../components" as Components

Components.ChatRoot {
    id: root
    chatList: chatListView

    property string chatBackgroundSelf: "#056162"
    property string chatBackgroundThem: "#262d31"
    property int itemHeightDefault: 68
    property int itemHeightSmall: 32

    historyPopupBackgroundColor: "#262d31"
    historyPopupTextColor: "white"

    Image {
        // background
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
        property int itemHeightSpacing: ctx.scaleFactor == 1 ? 12 : 32

        delegate: RowLayout {
            id: item

            property int itemHeight: textColumn.implicitHeight + chatListView.itemHeightSpacing
            property bool highlight: root.highlightEventId == event_id

            height: itemHeight + 12
            width: parent.width
            spacing: 0

            Item {
                visible: outgoing
                Layout.fillWidth: true
                Layout.preferredHeight: 32
            }

            Rectangle {
                color: highlight ? "white" : "transparent"
                Layout.preferredHeight: itemHeight
                Layout.preferredWidth: {
                    var max_width = item.width / 6 * 4;
                    var meta_width = metaRow.implicitWidth + 32;
                    var text_width = textMessage.implicitWidth + 32;

                    if(meta_width > text_width)
                        if(meta_width < max_width) return meta_width;

                    if(text_width < max_width) return text_width;
                    return max_width;
                }

                Rectangle {
                    id: textRectangle
                    radius: highlight ? 0 : 4
                    clip: true
                    color: outgoing ? root.chatBackgroundSelf : root.chatBackgroundThem
                    anchors.fill: parent
                    anchors.margins: highlight ? 2 * ctx.scaleFactor : 0

                    ColumnLayout {
                        id: textColumn
                        anchors.fill: parent
                        anchors.margins: 6 * ctx.scaleFactor
                        anchors.leftMargin: 10 * ctx.scaleFactor
                        anchors.rightMargin: 10 * ctx.scaleFactor
                        Layout.fillWidth: true
                        Layout.preferredHeight: itemHeight

                        RowLayout {
                            id: metaRow
                            spacing: 8

                            Item {
                                visible: !isHead
                                Layout.fillWidth: true
                            }

                            Components.PlainText {
                                visible: !outgoing && isHead
                                font.pointSize: 12 * ctx.scaleFactor
                                color: "lightblue"
                                text: name
                            }

                            Item {
                                visible: isHead
                                Layout.fillWidth: true
                            }

                            Components.PlainText {
                                font.pointSize: 12 * ctx.scaleFactor
                                color: "#98ac90"
                                text: datestr + " " + hourstr
                                Layout.rightMargin: 0
                            }
                        }

                        Components.PlainText {
                            id: textMessage
                            color: "white"
                            text: message
                            wrapMode: Text.WordWrap
                            width: parent.width
                            font.pointSize: 14 * ctx.scaleFactor
                            Layout.preferredWidth: parent.width
                        }
                    }
                }
            }

            Item {
                visible: !outgoing
                Layout.fillWidth: true
                Layout.preferredHeight: 32
            }
        }
    }
}
