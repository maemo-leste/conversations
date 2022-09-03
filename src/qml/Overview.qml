import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0
import MaemoConfig 1.0

import "../components" as Components

Rectangle {
    id: root
    property string highlight: "#00a2ff"
    property int itemHeight: 76 * ctx.scaleFactor
    property int topBarHeight: 54 * ctx.scaleFactor
    color: "black"

    ListView {
        height: root.topBarHeight
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        z: parent.z + 1

        model: overviewServiceModel
        orientation: Qt.Horizontal
        layoutDirection: Qt.LeftToRight

        delegate: Rectangle {
            color: "black"
            width: text.implicitWidth + 75
            height: root.topBarHeight

            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width * 0.8
                height: parent.height * 0.7
                spacing: 0

                Item {
                    Layout.preferredHeight: parent.height * 0.8
                    Layout.fillWidth: true

                    Text {
                        id: text
                        text: name || "";
                        font.pointSize: 18 * ctx.scaleFactor
                        color: "white"
                        anchors.centerIn: parent
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 4
                    Layout.fillWidth: true
                    color: "transparent"
                    visible: overviewServiceModel.activeIndex == index

                    Image {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        horizontalAlignment: Image.AlignLeft

                        height: parent.preferredHeight
                        source: "qrc:///overviewdots.png"
                        fillMode: Image.TileHorizontally
                        smooth: false
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    overviewServiceModel.changeActiveIndex(index);
                }
            }
        }
    }

    ListView {
        anchors.fill: parent
        anchors.topMargin: root.topBarHeight
        anchors.bottomMargin: 10
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        model: chatOverviewModel
        boundsBehavior: Flickable.StopAtBounds

        delegate: Rectangle {
            height: root.itemHeight
            width: parent.width
            color: "black"

            Item {
                height: root.itemHeight
                width: parent.width

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: !ctx.isMaemo
                    cursorShape: Qt.PointingHandCursor

                    onEntered: {
                        if(!ctx.isMaemo)
                            parent.parent.color = highlight;
                    }
                    onExited: {
                        if(!ctx.isMaemo)
                            parent.parent.color = "black";
                    }
                    onClicked: {
                        appWindow.overviewRowClicked(group_uid, local_uid, remote_uid, "", service_id);
                    }
                }
            }

            RowLayout {
                spacing: 0
                height: root.itemHeight
                width: parent.width

                Item {
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: root.itemHeight

                    Image {
                        width: 48
                        height: 48
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        id: imgStatus
                        source: ctx.ossoIconLookup(icon_name + ".png");
                        visible: true
                    }
                }

                Item {
                    // spacer
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: root.itemHeight
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0
                    clip: true

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.itemHeight / 2

                        Components.PlainText {
                            text: name
                            color: "white"
                            font.pointSize: 18 * ctx.scaleFactor
                            Layout.alignment: Qt.AlignTop
                        }

                        Components.PlainText {
                            Layout.leftMargin: 6
                            text: {
                                if(message !== "") datestr + " " + hourstr;
                                else "";
                            }
                            color: "grey"
                            font.pointSize: 12 * ctx.scaleFactor
                            Layout.alignment: Qt.AlignTop
                        }

                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    Components.PlainText {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.itemHeight / 2
                        color: "grey"
                        font.pointSize: 14 * ctx.scaleFactor
                        text: {
                            if(message !== "") message;
                            else datestr;
                        }
                    }
                }

                Item {
                    Layout.preferredHeight: root.itemHeight
                    Layout.preferredWidth: root.itemHeight

                    Image {
                        width: 48
                        height: 48
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        id: imgPerson
                        source: ctx.ossoIconLookup("general_default_avatar.png")
                        smooth: true
                        visible: true
                    }
                }
            }
        }
    }

    function onPageCompleted() {

    }
}
