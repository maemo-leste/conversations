import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0


ListView {
    id: root
    model: chatModel

    signal scrollToBottom()

    //boundsBehavior: Flickable.StopAtBounds
    property var chatScroll: chatScroll
    property bool scrollable: root.childrenRect.height > parent.height
    property bool atBottom: (chatScroll.position + chatScroll.size) >= 1
    property bool atTop: chatScroll.position <= 0.01
    property bool mayAutoScroll: atBottom && scrollable

    onCountChanged: {  // scroll to bottom
        //if(chatListView.mayAutoScroll)
        scrollToBottom();
    }

    ScrollBar.vertical: ScrollBar {
        id: chatScroll
        visible: false
    }
}