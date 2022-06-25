import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.15

Rectangle {
    color: "black"
    anchors.fill: parent

    Item {
        id: ctx
        property int scaleFactor: 1.0
    }

    ListModel {
        id: chatSearchModel
        ListElement {
            name: "John Brown"
            datestr: "18 Apr"
            msg: "hello bla, <span style=\"background: white;color:#056162\">dolor</span> bla"
        }
        ListElement {
            name: "Sam Wise"
            datestr: "14 Apr"
            msg: "Lorem ipsum <span style=\"background: white;color:#056162\">dolor</span> sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat."
        }
    }

    signal itemClicked(string group_uid, string local_uid, string remote_uid, string event_id);

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 40
        anchors.leftMargin: 40
        anchors.rightMargin: 40

        Rectangle {
            radius: 8

            clip: true
            Layout.preferredHeight: 64
            Layout.preferredWidth: parent.width
            color: "#313133"

            RowLayout {
                id: box
                spacing: 0
                anchors.fill: parent

                Item {
                    Layout.preferredWidth: 64
                    Layout.fillHeight: true
                    Layout.leftMargin: 8

                    Image {
                        source: "https://i.imgur.com/IAEEPrf.png"
                        anchors.centerIn: parent
                        width: 26
                        height: 40
                    }
                }

                property string placeholder: "Search"
                property bool hasInput: searchBox.text !== ""

                TextInput {
                    id: placeholderText
                    Layout.preferredWidth: parent.width * 0.75
                    Layout.fillHeight: true
                    text: box.hasInput ? "" : "Search"

                    color: "grey"
                    font.pixelSize: 32

                    verticalAlignment: TextInput.AlignVCenter
                    leftPadding: 20
                    readOnly: true

                    TextInput {
                        id: searchBox
                        text: ""
                        anchors.fill: parent

                        color: "white"
                        font.pixelSize: 32

                        verticalAlignment: TextInput.AlignVCenter
                        leftPadding: 20

                        onActiveFocusChanged: {
                            if (activeFocus){
                                if (box.enteredText === "") {
                                    searchBox.text = "";
                                }
                            }
                        }

                        onEditingFinished: {
                            box.enteredText = searchBox.text;
                            if (searchBox.text === "") {
                                searchBox.text = searchBox.placeholder
                            }
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

                Item {
                    Layout.preferredWidth: 72
                    Layout.fillHeight: true
                    Layout.rightMargin: 8

                    Image {
                        source: box.hasInput ? "https://i.imgur.com/9oa788Z.png" : "https://i.imgur.com/02a4nlq.png"
                        anchors.centerIn: parent
                        width: 26
                        height: 26
                    }
                }
            }

            Layout.bottomMargin: 32
        }

        ListView {
            spacing: 20
            Layout.preferredHeight: childrenRect.height
            Layout.fillWidth: true

            model: chatSearchModel
            delegate: ColumnLayout {
                spacing: 8 * ctx.scaleFactor
                id: item
                width: parent.width
                height: implicitHeight

                TextInput {
                    id: metaBox
                    text: "Test name" + " - " + "12/12/2009" + " " + "12:35"
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

                    Text {
                        id: textBubble
                        anchors.fill: parent
                        anchors.margins: 14 * ctx.scaleFactor
                        wrapMode: Text.WordWrap
                        text: msg;
                        color: "white"
                        font.pointSize: 18 * ctx.scaleFactor
                        textFormat: Text.RichText
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.itemClicked(group_uid, local_uid, remote_uid, event_id);
                        }
                    }
                }
            }
        }

        Text {
            id: noResultsText
            text: "No results."
            color: "grey"
            font.pointSize: 18 * ctx.scaleFactor
            textFormat: Text.RichText
            visible: chatSearchModel.count === 0
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
