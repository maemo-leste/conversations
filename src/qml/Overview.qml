import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0
import MaemoConfig 1.0

import "../components" as Components

Rectangle {
    id: root
    property string highlight: "#00a2ff"
    property int itemHeight: 76 * ctx.scaleFactor
    color: "black"

    ListView {
        anchors.fill: parent
        anchors.topMargin: 12
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
                        appWindow.overviewRowClicked(group_uid, local_uid, remote_uid, "");
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
                            text: remote_name
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
