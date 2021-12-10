import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

import "." as Components

Rectangle {
    id: chatRoot
    visible: true
    color: "grey"

    property var chatList
    property string historyPopupBackgroundColor: "#b2b2b2"
    property string historyPopupTextColor: "#393939"
    signal scrollToBottom()
    signal fetchHistory()

    onScrollToBottom: scrollBottomTimer.start()

    Components.ChatScrollToBottomButton {
        z: parent.z + 1
        visible: !chatList.atBottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 60
        anchors.bottomMargin: 60
        onClicked: scrollBottomTimer.start();
    }

    Rectangle {
        // detect requesting history - endless scroll
        visible: chatList.chatScroll.position < 0.0
        opacity: Math.abs(chatList.chatScroll.position * 10)
        z: parent.z + 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 20

        width: parent.width / 3
        height: 32 * ctx.scaleFactor
        color: chatRoot.historyPopupBackgroundColor
        radius: 6

        Text {
            color: chatRoot.historyPopupTextColor
            font.pointSize: 16 * ctx.scaleFactor
            text: !chatModel.exhausted ? "Load history" : "No more history"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }

        Item {
            visible: chatList.chatScroll.position <= -0.1
            onVisibleChanged: {
                if(visible && !chatModel.exhausted && chatListView.count >= chatModel.limit)
                    fetchHistory();
            }
        }
    }

    Rectangle {
        color: "#80000000"
        z: parent.z + 1
        visible: ctx.isDebug
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 60
        anchors.topMargin: 60
        height: debugBar.childrenRect.height
        width: debugBar.childrenRect.width

        ColumnLayout {
            // debugBar
            id: debugBar
            property int pointSize: 16

            Text {
                color: "lime"
                text: "Messages: " + chatList.count + " (limit: " + chatModel.limit + " offset: " + chatModel.offset + " exhausted: " + chatModel.exhausted + ")"
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Text {
                color: "lime"
                text: "mayAutoScroll: " + chatList.mayAutoScroll
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Text {
                color: "lime"
                text: "atBottom: " + chatList.atBottom
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Text {
                color: "lime"
                text: "atTop: " + chatList.atTop
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Text {
                color: "lime"
                text: "rootHeight: " + chatRoot.height
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Text {
                color: "lime"
                text: "chatList.cRect.height: " + chatList.childrenRect.height
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Text {
                color: "lime"
                text: "scrollPosition: " + chatList.chatScroll.position.toFixed(4)
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Text {
                color: "lime"
                text: "scrollable: " + chatList.scrollable
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Text {
                color: "lime"
                text: "scaling: " + ctx.scaleFactor
                font.pointSize: parent.pointSize
                font.bold: true
            }
        }
    }

    Timer {
        id: scrollBottomTimer
        interval: 10
        repeat: false
        running: false
        onTriggered: chatList.positionViewAtEnd();
    }

    onFetchHistory: {
        // Prepend new items to the model by calling `getPage()`.
        // temp. disable visibility to 'break' the touch gesture,
        // if we dont the list scrolling bugs out by "jumping"
        chatList.visible = false;
        var count_results = chatModel.getPage();
        if(!chatList.atBottom) {
            var jump_to = count_results <= 1 ? 0 : count_results - 1
            chatList.positionViewAtIndex(jump_to, ListView.Beginning)
        }

        chatList.visible = true;
    }

    Component.onCompleted: {
        root.scrollToBottom();
    }
}