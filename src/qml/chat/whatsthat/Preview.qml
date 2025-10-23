import QtQuick 2.15
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.15

import Conversations 1.0
import "../components" as Components

Rectangle {
    id: previewRoot
    property var model
    property int loadingIconFrameIndex: 1

    height: previewRoot.model.totalHeight
    signal itemHeightChanged();

    Connections {
        target: previewRoot.model

        function onTotalHeightChanged() {
            previewRoot.itemHeightChanged();
        }
    }

    color: "transparent"

    MouseArea {
        anchors.fill: parent
        onClicked: {
            preview.buttonPressed(true);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // preview button - user interaction
        Rectangle {
            color: "transparent"
            visible: previewRoot.model.displayButton
            Layout.preferredHeight: ctx.scaleFactor !== 1.0 ? 64 : previewRoot.model.buttonHeight
            Layout.fillWidth: true

            RowLayout {
                id: previewContainer
                anchors.fill: parent

                RowLayout {
                    spacing: 8

                    ColumnLayout {
                        spacing: 0
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Rectangle {
                            color: "transparent"
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                font.pointSize: ctx.scaleFactor !== 1.0 ? 16 : 14;
                                font.bold: true
                                font.family: fixedFont
                                wrapMode: hardWordWrap ? Text.WrapAnywhere : Text.WordWrap
                                color: "white"
                                text: {
                                    let linksText = weblinks_count > 1 ? "links" : "link";
                                    if (previewRoot.model.state === 0) {
                                        return "Preview " + weblinks_count + " " + linksText;
                                    } else if (previewRoot.model.state === 1) {
                                        return "Fetching: " + weblinks_count + " " + linksText;
                                    } else if (previewRoot.model.state === 2) {
                                        return "Fetching HEAD done";
                                    } else if (previewRoot.model.state === 3) {
                                        return "Fetching: " + previewRoot.model.downloadProgress + "%";
                                    } else if (previewRoot.model.state === 4) {
                                        return "Error. Retry?";
                                    }

                                    return "[debug me]"
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: preview.buttonPressed(true);
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                        Layout.preferredWidth: ctx.scaleFactor !== 1.0 ? 60 : 42;

                        Image {
                            visible: previewRoot.model.state !== 3
                            width: ctx.scaleFactor !== 1.0 ? 46 : 32;
                            height: ctx.scaleFactor !== 1.0 ? 46 : 32;
                            anchors.centerIn: parent
                            source: "qrc:/Shortcut.svg"
                            sourceSize: Qt.size(width, height)
                            cache: true
                        }

                        Image {
                            id: previewProgressIcon
                            visible: previewRoot.model.state === 3
                            anchors.centerIn: parent
                            height: 32 * ctx.scaleFactor
                            width: 32 * ctx.scaleFactor
                            source: "qrc:/Progress%1.svg".arg(previewRoot.loadingIconFrameIndex)
                            sourceSize: Qt.size(width, height)

                            Timer {
                                id: progressTimer
                                interval: 100
                                running: previewRoot.model.state === 3
                                repeat: true
                                onTriggered: {
                                    if (previewRoot.loadingIconFrameIndex < 8) {
                                        previewRoot.loadingIconFrameIndex++;
                                    } else {
                                        previewRoot.loadingIconFrameIndex = 1;
                                    }
                                    previewProgressIcon.source = "qrc:/Progress%1.svg".arg(previewRoot.loadingIconFrameIndex);
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: preview.buttonPressed(true);
                        }
                    }
                }
            }
        }

        // results
        Rectangle {
            color: "#1f2528"
            Layout.preferredHeight: previewRoot.model.resultsHeight * ctx.scaleFactor

            Layout.fillWidth: true
            visible: previewRoot.model.resultsHeight !== 0

            ListView {
                id: previewListView
                anchors.fill: parent
                model: previewRoot.model
                interactive: false
                spacing: 2

                delegate: Rectangle {
                    width: parent.width
                    height: itemHeight * ctx.scaleFactor
                    color: "transparent"

                    // text
                    Loader {
                        id: previewTextLoader
                        anchors.fill: parent

                        visible: displayType === 0 || (displayType === 1 && !previewRoot.model.inlineImages)
                        active: displayType === 0 || (displayType === 1 && !previewRoot.model.inlineImages)

                        sourceComponent: Component {
                            PreviewText {
                                id: previewTextComponent
                                title: titleRole
                                subtitle: subtitleRole
                            }
                        }

                        onLoaded: {
                            // console.log("PREVIEW TEXT");
                        }
                    }

                    // image
                    Loader {
                        id: previewImageLoader
                        anchors.fill: parent

                        visible: displayType === 1 && previewRoot.model.inlineImages
                        active: displayType === 1 && previewRoot.model.inlineImages

                        sourceComponent: Component {
                            PreviewImage {
                                id: previewImageComponent
                                previewFilePath: filepath
                            }
                        }

                        onLoaded: {
                            // console.log("PREVIEW IMAGE");
                        }
                    }
                }
            }
        }
    }
}
