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
    property string highlightEventId: ""
    property int atTopHeight: 0
    signal scrollToBottom()
    signal scrollToBottomFinished()
    signal fetchHistory()

    property bool chatPostReady: false
    signal chatPreReady();

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

        Components.PlainText {
            color: chatRoot.historyPopupTextColor
            font.pointSize: 16 * ctx.scaleFactor
            text: !chatModel.exhausted ? "Load history" : "No more history"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }

        Item {
            // endless-scroll: fetch new historic messages when user scrolls up
            visible: {
                var heightMargin = chatRoot.height / 8;
                return chatList.chatScroll.position <= -0.06 || (
                       chatList.atTop &&
                       root.atTopHeight != 0 &&
                       chatRoot.height >= 0 &&
                       chatList.childrenRect.height >= (root.atTopHeight + heightMargin))
            }
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

            Components.PlainText {
                color: "lime"
                text: "Messages: " + chatList.count + " (limit: " + chatModel.limit + " offset: " + chatModel.offset + " exhausted: " + chatModel.exhausted + ")"
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "mayAutoScroll: " + chatList.mayAutoScroll
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "atBottom: " + chatList.atBottom
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "atTop: " + chatList.atTop
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "rootHeight: " + chatRoot.height
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "chatList.cRect.height: " + chatList.childrenRect.height
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "scrollPosition: " + chatList.chatScroll.position.toFixed(4)
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "scrollPressed: " + chatList.chatScroll.pressed
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "scrollable: " + chatList.scrollable
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "atTopHeight: " + root.atTopHeight
                font.pointSize: parent.pointSize
                font.bold: true
            }

            Components.PlainText {
                color: "lime"
                text: "scaling: " + ctx.scaleFactor
                font.pointSize: parent.pointSize
                font.bold: true
            }
        }
    }

    Timer {
        id: chatPreReadyTimer
        interval: 50
        repeat: false
        running: false
        onTriggered: root.chatPreReady();
    }

    Timer {
        id: scrollBottomTimer
        interval: 10
        repeat: false
        running: false
        onTriggered: {
            console.log('scrollBottomTimer() fired');
            while(!chatList.atBottom)
              chatList.positionViewAtEnd();
            chatRoot.scrollToBottomFinished();
        }
    }

    Timer {
        // we can get rid of this timer and call `positionViewAtIndex()` directly
        // when this is looked into;
        //   interesting stack window behavior ('X-Maemo-StackedWindow') where you have
        //   window #1 and window #2, you close #2, ~400ms transition animation to #1, if
        //   you 'change' something visually in window #1 during the transition, those changes
        //   are not being shown after the transition completes. when you start interacting
        //   with #1, by for example registering a touch gesture, the window 'refreshes' and
        //   the change is there. So I was thinking maybe Maemo StackedWindow code needs to call
        //   `refresh()` or `redraw()` on that #1 window when the transition is complete.
        id: scrollIdxTimer
        property int idx: -1
        interval: 350
        repeat: false
        running: false
        onTriggered: {
            if(idx == -1) return;
            console.log('scrollIdxTimer() fired, jumping to', idx);
            chatListView.positionViewAtIndex(idx, ListView.Center);
        }
    }

    onFetchHistory: {
        // Prepend new items to the model by calling `getPage()`.
        // temp. disable visibility to 'break' the touch gesture,
        // if we dont the list scrolling bugs out by "jumping"
        console.log('onFetchHistory();');
        chatList.visible = false;
        var count_results = chatModel.getPage();
        if(!chatList.atBottom) {
            var jump_to = count_results <= 1 ? 0 : count_results - 1;
            chatList.positionViewAtIndex(jump_to, ListView.Beginning);
        }

        chatList.visible = true;
    }

    Item {
        // hack to emit signal when root.atTop changes
        visible: chatListView.atTop
        onVisibleChanged: {
            // console.log('atTop', chatListView.atTop);
            root.atTopHeight = chatList.childrenRect.height;
        }
    }

    Connections {
        target: chatWindow
        onScrollDown: {
            console.log('Connections: onScrollDown()');
            while(!chatList.atBottom)
              chatList.positionViewAtEnd();
        }
        onJumpToMessage: {
            console.log('Connections: onJumpToMessage()', event_id);

            chatRoot.highlightEventId = event_id;
            var highlightIdx = chatModel.eventIdToIdx(event_id);

            scrollIdxTimer.idx = highlightIdx;
            scrollIdxTimer.start();
        }
        onChatPreReady: {
            console.log('onChatPreReady()');
            chatRoot.chatPostReady = false;
        }
        onChatPostReady: {
            console.log('onChatPostReady()');
            chatRoot.chatPostReady = true;
        }
    }

    Component.onCompleted: {
        console.log('ChatRoot onCompleted()');
        chatPreReadyTimer.start();
    }
}