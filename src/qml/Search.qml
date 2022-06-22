import QtQuick 2.11
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.10
import MaemoConfig 1.0


Rectangle {
    id: root
    color: "black"

    signal itemClicked(string uid);

    ListModel {
        id: testModel
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

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 40
        anchors.leftMargin: 40
        anchors.rightMargin: 40

        Rectangle {
            radius: 8

            clip: true
            Layout.preferredHeight: 64 * ctx.scaleFactor
            Layout.preferredWidth: parent.width
            color: "#313133"

            RowLayout {
                id: box
                spacing: 0
                anchors.fill: parent

                Item {
                    Layout.preferredWidth: 64 * ctx.scaleFactor
                    Layout.fillHeight: true
                    Layout.leftMargin: 8 * ctx.scaleFactor

                    Image {
                        source: "qrc:///back.png"
                        anchors.centerIn: parent
                        width: 26 * ctx.scaleFactor
                        height: 40 * ctx.scaleFactor
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            searchBox.text = "";
                            view.stackReplace(appWindow.overviewWindow, "", 0);
                        }
                    }
                }

                property string placeholder: "Search"
                property bool hasInput: searchBox.text !== ""

                TextInput {
                    id: placeholderText
                    Layout.preferredWidth: parent.width * 0.6
                    Layout.fillHeight: true
                    text: box.hasInput ? "" : "Search"

                    color: "grey"
                    font.pixelSize: 32 * ctx.scaleFactor

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

                        onEditingFinished: {
                            if (searchBox.text === "") {
                                searchBox.text = box.placeholder
                            }
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
                        }
                    }
                }
            }

            Layout.bottomMargin: 32 * ctx.scaleFactor
        }

        ListView {
            spacing: 20
            Layout.preferredHeight: childrenRect.height
            Layout.fillWidth: true

            model: testModel
            delegate: ColumnLayout {
                spacing: 8 * ctx.scaleFactor
                id: item
                width: parent.width
                height: item.implicitHeight

                TextInput {
                    id: metaBox
                    text: name + " - " + datestr
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
                        text: msg
                        color: "white"
                        font.pointSize: 18 * ctx.scaleFactor
                        textFormat: Text.RichText
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.itemClicked('uid');
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}