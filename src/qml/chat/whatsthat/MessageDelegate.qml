import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.15

import "../components" as Components

RowLayout {
    id: item
    width: ListView.view.width
    property int itemMaxWidth: width / 6 * 3
    spacing: 0

    signal showMessageContextMenu(int event_id, var point);

    property bool highlight: false
    property int screenHeight: 0
    property int screenWidth: 0
    property int avatarSize: 58
    property alias chatBgShader: shaderEffect
    property color gColorStart: "#363e42"
    property color gColorEnd: "#056162"
    property var gColorStartVec: Qt.vector3d(gColorStart.r, gColorStart.g, gColorStart.b)
    property var gColorEndVec: Qt.vector3d(gColorEnd.r, gColorEnd.g, gColorEnd.b)

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
            width_meta_row += metaDateText.implicitWidth;
        else
            width_meta_row += metaNameText.implicitWidth + metaDateText.implicitWidth;
        width_meta_row += 12;

        let width_message = textItem.implicitWidth + 24;
        let width_result = 70; // minimum

        // console.log(message);

        if(displayAvatar && hasAvatar) {
            width_result += avatarSize;
            // console.log("+= avatarSize", message);
        }

        if(has_meta_row) {
            width_result += width_meta_row;
            // console.log("+= width_meta_row", width_meta_row);
        }

        if(width_message > width_result) {
            if(width_message < max_width) {
                // console.log("return", width_message);
                return width_message;
            }

            // console.log("max_width", max_width);
            return max_width;
        }

        return width_result;
    }

    property bool displayAvatar: !outgoing && (isHead || display_timestamp) && !chatWindow.groupchat && ctx.displayAvatars && hasAvatar

    // shader
    Connections {
        target: root

        function onChatBgShaderUpdate() {
            if(ctx.displayChatGradient)
                shaderEffect.setGlobalY();
        }
    }

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

    height: {
        let meta_height = 12 * ctx.scaleFactor;
        if(!isHead && !display_timestamp)
            meta_height = -12 * ctx.scaleFactor;

        let _height = textItem.implicitHeight + textItem.font.pointSize + (20) + meta_height;
        let _height_and_avatar = avatarContainer.childrenRect.height + (20 ) + meta_height;

        if (displayAvatar) {
            if (_height_and_avatar > _height) {
                if(textItem.text.length <= 20)  // hack
                    return _height_and_avatar - 10;

                return _height_and_avatar;
            }
            return _height;
        } else {
            return _height;
        }
    }

    Rectangle {
        visible: outgoing
        color: "transparent"
        Layout.preferredWidth: itemWidth
        Layout.fillHeight: true
    }

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

        ShaderEffect {
            visible: ctx.displayChatGradient && outgoing
            id: shaderEffect
            anchors.fill: parent

            function setGlobalY() {
                let _y = shaderEffect.mapToGlobal(Qt.point(0, 0)).y;
                if(_y < 0)  _y = 0.0;
                else if(_y > item.screenHeight) _y = item.screenHeight;
                shaderEffect.globalY = _y;
            }

            fragmentShader: "
                    uniform lowp float qt_Opacity;
                    uniform highp vec2 resolution;
                    uniform highp float globalY;
                    uniform lowp vec3 gradientStart;
                    uniform lowp vec3 gradientEnd;

                    void main() {
                        highp float normalizedY = globalY / resolution.y;
                        highp float gradientPosition = clamp((normalizedY - 0.15) / (0.85 - 0.15), 0.0, 1.0);
                        lowp vec3 color = mix(gradientStart, gradientEnd, gradientPosition);
                        gl_FragColor = vec4(color, qt_Opacity);
                    }
                "

            // uniforms
            property var resolution: Qt.size(root.width, root.height)
            property real globalY: shaderEffect.setGlobalY();
            property var gradientStart: item.gColorStartVec
            property var gradientEnd: item.gColorEndVec
        }

        Rectangle {
            color: "transparent"
            anchors.fill: parent
            anchors.topMargin: 10
            anchors.leftMargin: (hasAvatar && displayAvatar) ? 6 : 12
            anchors.rightMargin: 12
            anchors.bottomMargin: 10

            RowLayout {
                anchors.fill: parent
                anchors.margins: 0
                width: parent.width
                height: parent.height

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
                        Layout.preferredHeight: 20 * ctx.scaleFactor

                        RowLayout {
                            anchors.fill: parent

                            Components.PlainText {
                                id: metaNameText
                                visible: (!outgoing && isHead) || !outgoing
                                color: root.colorTextThem
                                text: name
                                font.pointSize: ctx.scaleFactor !== 1.0 ? 16 : 14;
                                font.bold: true
                                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                            }

                            Components.PlainText {
                                id: metaDateText
                                color: outgoing ? root.colorTextSelf : root.colorTextThem
                                font.pointSize: ctx.scaleFactor !== 1.0 ? 14 : 12;
                                font.family: fixedFont
                                text: datestr + " " + hourstr
                                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                                opacity: 0.6
                            }
                        }
                    }

                    Components.PlainText {
                        id: textItem
                        color: outgoing ? root.colorTextSelf : root.colorTextThem
                        text: message
                        font.pointSize: 14 * ctx.scaleFactor
                        wrapMode: hardWordWrap ? Text.WrapAnywhere : Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Item {
                        // visible: !hasAvatar
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }
        }

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

    Rectangle {
        visible: !outgoing
        color: "transparent"
        Layout.preferredWidth: itemWidth
        Layout.fillHeight: true
    }
}
