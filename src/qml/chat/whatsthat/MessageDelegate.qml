import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

import "../components" as Components

RowLayout {
    id: item
    property int itemHeightSpacing: ctx.scaleFactor == 1 ? 12 : 18
    property int itemHeight: {
      if(!chat_event) return 18;  // join/leave
      return textColumn.implicitHeight + item.itemHeightSpacing;  // normal msg
    }
    property bool highlight: false

    // handy snippet to determine if this current delegate is in view, in case
    // we need it in the future
    // property int yoff: Math.round(item.y - chatListView.contentY)
    // property bool isFullyVisible: {
    //     let _visible = yoff > chatListView.y && yoff + height < chatListView.y + chatListView.height;
    //     return _visible;
    // }

    height: itemHeight + 12
    width: parent !== null ? parent.width : 0
    spacing: 0

    // (group)join, leave events, etc. are displayed differently
    Item {
        visible: !chat_event && ctx.displayGroupchatJoinLeave
        Layout.preferredHeight: 18
        Layout.fillWidth: true

        RowLayout {
            anchors.fill: parent
            Layout.preferredHeight: 18
            Layout.fillWidth: true
            spacing: 32

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 18
            }

            Components.PlainText {
                Layout.preferredHeight: 18
                Layout.alignment: Qt.AlignHCenter

                text: message
                color: root.colorTextSelf
                wrapMode: Text.WordWrap
                font.pointSize: 14
                font.bold: false
            }

            Components.PlainText {
                Layout.preferredHeight: 18
                Layout.alignment: Qt.AlignHCenter

                text: datestr + " " + hourstr
                color: root.colorTextSelf
                wrapMode: Text.WordWrap
                font.pointSize: 14
                font.bold: false
                opacity: 0.7
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 18
            }
        }
    }

    Item {
        visible: outgoing && chat_event
        Layout.fillWidth: true
        Layout.preferredHeight: 32
    }

    Rectangle {
        visible: chat_event
        color: highlight ? colorHighlight : "transparent"
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
                    visible: isHead || display_timestamp

                    Components.PlainText {
                        visible: (!outgoing && isHead) || display_timestamp
                        font.pointSize: 14 * ctx.scaleFactor
                        font.bold: true
                        color: root.colorTextThem
                        text: name
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Components.PlainText {
                        visible: display_timestamp
                        font.pointSize: 12 * ctx.scaleFactor
                        color: outgoing ? root.colorTextSelf : root.colorTextThem
                        text: datestr + " " + hourstr
                        Layout.rightMargin: 0
                        opacity: 0.6
                    }
                }

                Components.PlainText {
                    id: textMessage
                    color: outgoing ? root.colorTextSelf : root.colorTextThem
                    text: message
                    wrapMode: hardWordWrap ? Text.WrapAnywhere : Text.WordWrap
                    width: parent.width
                    font.pointSize: 14 * ctx.scaleFactor
                    Layout.preferredWidth: parent.width
                }
            }
        }
    }

    Item {
        visible: !outgoing && chat_event
        Layout.fillWidth: true
        Layout.preferredHeight: 32
    }
}
