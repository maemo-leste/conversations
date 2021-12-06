import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0
import MaemoConfig 1.0

import "components" as Components

Rectangle {
    id: appWindow
    visible: true
    color: "black"
    property string highlight: "#00a2ff"
    property int itemHeight: 76 * ctx.scaleFactor

    signal rowClicked(string group_uid, string local_uid, string remote_uid);

    ListView {
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        anchors.fill: parent
        model: chatOverviewModel
        boundsBehavior: Flickable.StopAtBounds

        delegate: Rectangle {
            height: itemHeight
            width: parent.width
            color: "black"

            Item {
                height: itemHeight
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
                        appWindow.rowClicked(group_uid, local_uid, remote_uid);
                    }
                }
            }

            RowLayout {
                spacing: 0
                height: itemHeight
                width: parent.width

                Item {
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: itemHeight

                    Image {
                        width: 48
                        height: 48
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        id: imgStatus
                        source: ctx.ossoIconLookup(icon_name + ".png");
                        smooth: true
                        visible: true
                    }
                }

                Item {
                    // spacer
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: itemHeight
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0
                    clip: true

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: itemHeight / 2

                        Text {
                            text: remote_name
                            textFormat: Text.PlainText
                            color: "white"
                            font.pointSize: 18 * ctx.scaleFactor
                            Layout.alignment: Qt.AlignTop
                        }

                        Text {
                            Layout.leftMargin: 6
                            text: {
                                if(message !== "") datestr + " " + hourstr;
                                else "";
                            }
                            color: "grey"
                            textFormat: Text.PlainText
                            font.pointSize: 12 * ctx.scaleFactor
                            Layout.alignment: Qt.AlignTop

                        }

                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.preferredHeight: itemHeight / 2
                        textFormat: Text.PlainText
                        color: "grey"
                        font.pointSize: 14 * ctx.scaleFactor
                        text: {
                            if(message !== "") message;
                            else datestr;
                        }
                    }
                }

                Item {
                    Layout.preferredHeight: itemHeight
                    Layout.preferredWidth: itemHeight

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
}
