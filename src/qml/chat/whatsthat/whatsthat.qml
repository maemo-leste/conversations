import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.15

import "../components" as Components
import "."

Components.ChatRoot {
    id: root
    chatList: chatListView
    color: colorBackground

    property string colorBackground: ctx.inheritSystemTheme ? theme.colors.defaultBackgroundColor : "black"
    property string chatBackgroundSelf: ctx.inheritSystemTheme ? theme.colors.notificationBackgroundColor : "#056162"
    property string chatBackgroundThem: ctx.inheritSystemTheme ? theme.colors.contentSelectionColor : "#363e42"

    property string colorTextSelf: ctx.inheritSystemTheme ? theme.colors.defaultTextColor : "white"
    property string colorTextThem: ctx.inheritSystemTheme ? theme.colors.paintedTextColor : "white"

    property string colorHighlight: ctx.inheritSystemTheme ? theme.colors.contentSelectionColor : "white"
    property string colorReversedTextColor: ctx.inheritSystemTheme ? theme.colors.reversedTextColor : "lightblue"
    property string colorReversedSecondaryTextColor: ctx.inheritSystemTheme ? theme.colors.reversedSecondaryTextColor : "lightblue"
    property string colorDate: ctx.inheritSystemTheme ? theme.colors.reversedPaintedTextColor : "#98ac90"

    property int itemHeightDefault: 68
    property int itemHeightSmall: 32

    historyPopupBackgroundColor: "#262d31"
    historyPopupTextColor: "white"

    signal showMessageContextMenu(int event_id, var point);
    signal chatBgShaderUpdate();

    Image {
        // background
        visible: !ctx.inheritSystemTheme && !chatWindow.bgMatrixRainEnabled
        source: "qrc:/whatsthat/bg.png"
        anchors.fill: parent
        fillMode: Image.Tile
    }

    Component {
        id: shaderMatrixRainComponent
        Components.ShaderMatrixRain {
            anchors.fill: parent
        }
    }

    Loader {
        id: shaderLoader
        anchors.fill: parent
        active: chatWindow.bgMatrixRainEnabled
        sourceComponent: chatWindow.bgMatrixRainEnabled ? shaderMatrixRainComponent : null
    }

    Components.ChatListView {
        id: chatListView

        spacing: 8

        anchors.fill: parent
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.leftMargin: 32
        anchors.rightMargin: 32

        onScrollToBottom: root.scrollToBottom();
        delegate: MessageDelegate {
            screenWidth: root.width
            screenHeight: root.height
            highlight: root.highlightEventId == event_id
            onShowMessageContextMenu: root.showMessageContextMenu(event_id, point);
            onItemHeightChanged: {
                if(chatWindow.isPinned) {
                    ctx.singleShot(100, () => {
                        chatListView.positionViewAtIndex(chatListView.model.count - 1, ListView.End);
                    });
                    return;
                }

                // auto-scroll to the end of this delegate after the height changes
                // but only when we are not pinned to the bottom
                if(root.chatPostReady && chatWindow.linkPreviewRequiresUserInteraction) {
                    ctx.singleShot(100, () => {
                        chatListView.positionViewAtIndex(index, ListView.End);
                    });
                }
            }
        }
    }

    Connections {
        target: root
        function onScrollToBottomFinished() {
            if(ctx.displayChatGradient)
                root.chatBgShaderUpdate();
        }
    }

    Connections {
        target: chatListView
        function onCountChanged() {
            if(ctx.displayChatGradient)
              root.chatBgShaderUpdate();
        }
    }

    Connections {
        target: chatWindow
        function onChatPostReady() {
            if(ctx.displayChatGradient)
                root.chatBgShaderUpdate();
        }
    }

    Timer {
        interval: 100
        running: chatListView.moving && ctx.displayChatGradient
        repeat: true
        onTriggered: {
            if(ctx.displayChatGradient)
                root.chatBgShaderUpdate();
        }
    }
}
