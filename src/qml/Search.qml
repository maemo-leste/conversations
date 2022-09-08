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

        Rectangle {
            Layout.preferredHeight: 84 * ctx.scaleFactor
            Layout.preferredWidth: parent.width
            color: "black"
            z: parent.z + 1

            Rectangle {
                radius: 8

                clip: true
                height: 64 * ctx.scaleFactor
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                anchors.bottom: parent.bottom
                color: "#313133"
                z: parent.z + 1

                RowLayout {
                    id: box
                    spacing: 0
                    anchors.fill: parent

                    property string placeholder: "Search"
                    property bool hasInput: searchBox.text !== ""

                    TextInput {
                        id: placeholderText
                        Layout.preferredWidth: parent.width * 0.6
                        Layout.fillHeight: true
                        text: box.hasInput ? "" : "Search"

                        color: "grey"
                        font.pixelSize: 22 * ctx.scaleFactor

                        verticalAlignment: TextInput.AlignVCenter
                        leftPadding: 20 * ctx.scaleFactor
                        readOnly: true

                        TextInput {
                            id: searchBox
                            text: ""
                            anchors.fill: parent

                            color: "white"
                            font.pixelSize: 32 * ctx.scaleFactor

                            verticalAlignment: TextInput.AlignVCenter
                            leftPadding: 20 * ctx.scaleFactor

                            onActiveFocusChanged: {
                                if (activeFocus){
                                    if (box.enteredText === "") {
                                        searchBox.text = "";
                                    }
                                }
                            }

                            onTextEdited: {
                                if(searchBox.text.length >= 3) {
                                    var term = searchBox.text.replace("%", "");
                                    chatSearchModel.searchMessages("%%" + term + "%%", searchWindow.remote_uid);
                                } else {
                                    chatSearchModel.clear();
                                }
                            }

                            onEditingFinished: {

                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                    }

                    Item {
                        Layout.preferredWidth: 72 * ctx.scaleFactor
                        Layout.fillHeight: true
                        Layout.rightMargin: 8 * ctx.scaleFactor

                        Image {
                            source: box.hasInput ? "qrc:///close.png" : "qrc:///glass.png"
                            anchors.centerIn: parent
                            width: 26 * ctx.scaleFactor
                            height: 26 * ctx.scaleFactor
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if(box.hasInput) {
                                    searchBox.text = "";
                                }

                                chatSearchModel.clear();
                            }
                        }
                    }
                }

                Layout.bottomMargin: 32 * ctx.scaleFactor
            }
        }

        ListView {
            id: searchListView
            spacing: 20
            Layout.topMargin: 10
            Layout.leftMargin: 20
            Layout.rightMargin: 20
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
                    text: remote_name + " - " + datestr + " " + hourstr
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
                        text: highlight(message, searchBox.text);
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
            Layout.leftMargin: 20
            Layout.rightMargin: 20
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
        searchBox.forceActiveFocus();
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
        searchBox.forceActiveFocus();
    }
}
