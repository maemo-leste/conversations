import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Conversations 1.0

import "../components" as Components
import "."

RowLayout {
    id: item
    width: ListView.view.width
    property int itemMaxWidth: {
        let segment_width = width / 6;
        if(width < 600) {  // we are probably in landscape
            return segment_width * 5;
        } else {
            return segment_width * 4;
        }
    }
    spacing: 0

    signal showMessageContextMenu(int event_id, var point);
    signal itemHeightChanged();

    property bool highlight: false
    property int avatarSize: 58

    TextMetrics {
        id: msgMetrics
        font: textItem.font
        text: message
    }

    TextMetrics {
        id: nameMetrics
        font: metaNameText.font
        text: name
    }

    // handy snippet to determine if this current delegate is in view, in case
    // we need it in the future
    // property int yoff: Math.round(item.y - chatListView.contentY)
    // property bool isFullyVisible: {
    //     let _visible = yoff > chatListView.y && yoff + height < chatListView.y + chatListView.height;
    //     return _visible;
    // }

    property int itemWidth: {
        let max_width = itemMaxWidth;
        let has_meta_row = isHead || display_timestamp;
        let width_meta_row = 0;
        if(outgoing)
            width_meta_row += metaDateText.contentWidth;
        else
            width_meta_row += nameMetrics.advanceWidth + metaDateText.contentWidth;
        width_meta_row += 12;

        let width_message = msgMetrics.advanceWidth + 24;
        let width_result = 30; // minimum

        let displayName = outgoing && isHead;
        if(displayName)
            width_result -= 16;
        if(outgoing && !isHead)
            width_result -= 16;

        if(displayAvatar && hasAvatar) {
            width_result += avatarSize;
            // console.log("+= avatarSize", message);
        }

        if(has_meta_row) {
            width_result += width_meta_row;
            // console.log("+= width_meta_row", width_meta_row);
        }

        let _maxWidth = (w) => {
            if(w >= max_width)
                return max_width;
            return w;
        }

        if(width_message > width_result) {
            if(width_message < max_width) {
                // console.log("return", width_message);
                return _maxWidth(width_message * ctx.scaleFactor);
            }

            if(width_result >= max_width)
                return _maxWidth(width_result * ctx.scaleFactor);

            return _maxWidth(max_width * ctx.scaleFactor);
        }

        return _maxWidth(width_result * ctx.scaleFactor);
    }

    property bool displayAvatar: !outgoing && (isHead || display_timestamp) && !chatWindow.groupchat && ctx.displayAvatars && hasAvatar

    Connections {
        target: chatWindow

        // on flickable movement end, reset chat msg container state
        function onAvatarChanged() {
            imgAvatar.reload();
        }
    }

    Connections {
        target: chatListView

        // on flickable movement end, reset chat msg container state
        function onMovementEnded() {
            textContainer.state = "off";
        }
    }

    function calculateItemHeight() {
        let meta_height = 16 * ctx.scaleFactor;
        if(!isHead && !display_timestamp)
            meta_height = -12 * ctx.scaleFactor;

        let new_day_offset = 0;
        if(new_day && !outgoing)
            new_day_offset += 32;

        let _height = textItem.implicitHeight + textItem.font.pointSize + (20) + meta_height + new_day_offset;
        let _height_and_avatar = avatarSize + (20 ) + meta_height + new_day_offset;

        if (displayAvatar) {
            if (_height_and_avatar > _height) {
                if(message.length <= 20)  // hack
                    return _height_and_avatar - 10;

                return _height_and_avatar;
            }
            return _height;
        } else {
            return _height;
        }
    }

    height: preview !== undefined ?
        (preview.totalHeight * ctx.scaleFactor) + item.calculateItemHeight() :
        item.calculateItemHeight()

    Rectangle {
        id: textContainer
        property string bgColor: outgoing ? root.chatBackgroundSelf : root.chatBackgroundThem
        color: bgColor
        radius: 4
        // spacing: 0
        Layout.preferredWidth: itemWidth
        Layout.fillHeight: true
        Layout.alignment: outgoing ? Qt.AlignRight : Qt.AlignLeft
        // Layout.bottomMargin: 10
        // Layout.topMargin: 10

        Rectangle {
            color: "transparent"
            anchors.fill: parent
            anchors.topMargin: 10
            anchors.leftMargin: (hasAvatar && displayAvatar) ? 6 : 12
            anchors.rightMargin: 12
            anchors.bottomMargin: 10

            RowLayout {
                anchors.fill: parent

                // avatar
                Item {
                    id: avatarContainer
                    visible: displayAvatar
                    // color: "yellow"
                    Layout.preferredWidth: avatarSize + 10
                    Layout.fillHeight: true

                    Image {
                        id: imgAvatar
                        width: avatarSize
                        height: avatarSize
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: visible && hasAvatar ? avatar : ""
                        cache: true

                        function reload() {
                            imgAvatar.source = "";
                            imgAvatar.source = avatar;
                            hasAvatar = true;
                            imgAvatar.visible = !outgoing && !chatWindow.groupchat && ctx.displayAvatars && hasAvatar;
                            // textContainer.Layout.preferredWidth = textContainer.calcPrefWidth();
                        }
                    }
                }

                ColumnLayout {
                    spacing: 6
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Item {
                        id: metaRow
                        visible: isHead || display_timestamp
                        Layout.fillWidth: true
                        Layout.preferredHeight: 24 * ctx.scaleFactor

                        RowLayout {
                            id: metaRowInner
                            anchors.fill: parent
                            spacing: 10

                            Components.EmojiText {
                                id: metaNameText
                                visible: (!outgoing && isHead) || !outgoing
                                color: root.colorTextThem
                                plainText: name
                                font.pointSize: {
                                    if(ctx.scaleFactor === 1.0) return 14;
                                    else if (ctx.scaleFactor <= 1.5) return 20;
                                    return 24;
                                }
                                font.bold: true
                                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                            }

                            Components.PlainText {
                                id: metaDateText
                                color: outgoing ? root.colorTextSelf : root.colorTextThem
                                font.pointSize: {
                                    if(ctx.scaleFactor === 1.0) return 12;
                                    else if (ctx.scaleFactor <= 1.5) return 14;
                                    return 16;
                                }
                                text: datestr + " " + hourstr
                                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                                opacity: 0.6
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: textItem.implicitHeight

                        Components.EmojiText {
                            id: textItem
                            width: parent.width
                            color: outgoing ? root.colorTextSelf : root.colorTextThem
                            plainText: message
                            font.pointSize: 14 * ctx.scaleFactor
                            wrapMode: hardWordWrap ? Text.WrapAnywhere : Text.WordWrap
                            horizontalAlignment: outgoing ? Text.AlignRight : Text.AlignLeft

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                onPressed: {
                                    textContainer.state = "on";
                                }
                                onReleased: {
                                    textContainer.state = "off";
                                }
                                onClicked: function (mouse) {
                                    if (mouse.button === Qt.RightButton)
                                        chatWindow.showMessageContextMenu(event_id, Qt.point(mouse.x, mouse.y))
                                }
                                onPressAndHold: function (mouse) {
                                    if (mouse.button === Qt.LeftButton /*&&
                                         mouse.source === Qt.MouseEventNotSynthesized*/) {
                                        chatWindow.showMessageContextMenu(event_id, Qt.point(mouse.x, mouse.y))
                                    }
                                }
                            }
                        }
                    }

                    Loader {
                        id: previewLoader
                        visible: status === Loader.Ready
                        active: typeof(preview) !== 'undefined'
                        Layout.fillWidth: true
                        Layout.preferredHeight: preview !== undefined ? preview.totalHeight * ctx.scaleFactor : 0;

                        signal itemHeightChanged();
                        onItemHeightChanged: item.itemHeightChanged();

                        sourceComponent: Component {
                            Preview {
                                id: previewComponent
                                model: preview
                                onItemHeightChanged: previewLoader.itemHeightChanged();
                            }
                        }

                        onLoaded: {
                            // console.log("preview component state", preview.state);
                        }
                    }

                    Item {
                        // visible: !hasAvatar
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }

    Rectangle {
        visible: new_day && !outgoing
        color: "transparent"
        Layout.fillHeight: true
        Layout.fillWidth: true

        Text {
            font.pointSize: ctx.scaleFactor !== 1.0 ? 14 : 12;
            anchors.top: parent.top
            anchors.topMargin: 10;
            anchors.left: parent.left
            anchors.leftMargin: 16;
            text: {
                const day = rawdate.getDate();
                const months = [
                    "January","February","March","April","May","June",
                    "July","August","September","October","November","December"];
                const month = months[rawdate.getMonth()];
                return day + " " + month;
            }
            color: "white"
        }
    }

    Rectangle {
        visible: !outgoing
        color: "transparent"
        Layout.preferredWidth: itemWidth
        Layout.fillHeight: true
    }
}
