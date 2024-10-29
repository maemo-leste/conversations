import QtQuick 2.11
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import MaemoConfig 1.0

import "../components" as Components

Rectangle {
    id: root
    color: "black"

    signal itemClicked(int idx);

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ListView {
            id: searchListView
            spacing: 20
            Layout.topMargin: 0
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            Layout.fillHeight: true
            Layout.fillWidth: true
            model: chatSearchModel
            visible: count > 0

            ScrollBar.vertical:ScrollBar {
                id: listView
                anchors.right: parent.right
                visible: true
            }

            delegate: ColumnLayout {
                spacing: 8 * ctx.scaleFactor
                id: item
                width: parent.width
                height: implicitHeight

                Components.PlainText {
                    id: metaBox
                    text: name + " - " + datestr + " " + hourstr
                    color: "white"
                    font.pointSize: 18 * ctx.scaleFactor

                    Layout.fillWidth: true
                    Layout.preferredHeight: 32 * ctx.scaleFactor
                }

                Rectangle {
                    id: textBox
                    color: "#056162"
                    radius: 8
                    Layout.preferredHeight: textBubble.implicitHeight + textBubble.font.pointSize + textBubble.anchors.margins
                    Layout.fillWidth: true
                    clip: true

                    Components.PlainText {
                        id: textBubble
                        anchors.fill: parent
                        anchors.margins: 14 * ctx.scaleFactor
                        wrapMode: Text.WordWrap
                        text: highlight(message, searchWindow.search_term);
                        color: "white"
                        font.pointSize: 18 * ctx.scaleFactor
                        textFormat: Text.RichText
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.itemClicked(index);
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }
            }
        }

        Components.PlainText {
            id: noResultsText
            text: "No results."
            color: "grey"
            font.pointSize: 18 * ctx.scaleFactor
            textFormat: Text.RichText
            visible: chatSearchModel.count === 0
            Layout.topMargin: 20
            Layout.leftMargin: 0
            Layout.rightMargin: 0
        }

        Item {
            visible: chatSearchModel.count == 0
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }

    function onPageCompleted() {
        // not called, left-over from inclusion in `StackViewFancy`
        chatSearchModel.clear();
    }

    function highlight(message, term) {
        // wraps 'term' with a <span>
        let spans = ["<span style=\"background: white; color:#056162\">", "</span>"];
        let re = new RegExp(term, 'gi');

        let idx = message.search(re);
        if (idx === -1) return;
        let length = term.length;

        let prefix = message.slice(0, idx);
        let _term = message.slice(idx, idx + length);

        let suffix = message.slice(idx + length);
        return prefix + spans[0] + _term + spans[1] + suffix;
    }

    Component.onCompleted: {
        chatSearchModel.clear();
    }
}
