import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0
import MaemoConfig 1.0

import "../components" as Components

Rectangle {
    id: root
    color: colorBackground

    property string colorBackground: ctx.inheritSystemTheme ? theme.colors.defaultBackgroundColor : "black"
    property string colorText: ctx.inheritSystemTheme ? theme.colors.defaultTextColor : "white"
    property string colorHighlight: ctx.inheritSystemTheme ? theme.colors.SelectionColor : "#00a2ff"
    property string colorMutedText: ctx.inheritSystemTheme ? theme.colors.disabledTextColor : "grey"
    property string colorSecondaryText: ctx.inheritSystemTheme ? theme.colors.secondaryTextColor : "grey"

    property int itemHeight: 76 * ctx.scaleFactor
    property int topBarHeight: 54 * ctx.scaleFactor

    signal overviewRowClicked(int idx);

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
            color: colorBackground
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
                        color: colorText
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
            width: parent !== null ? parent.width : 0
            color: colorBackground

            Item {
                height: root.itemHeight
                width: parent.width

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: !ctx.isMaemo
                    cursorShape: Qt.PointingHandCursor

                    onEntered: {
                        if(!ctx.isMaemo)
                            parent.parent.color = colorHighlight;
                    }
                    onExited: {
                        if(!ctx.isMaemo)
                            parent.parent.color = colorBackground;
                    }
                    onClicked: {
                        overviewRowClicked(index);
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
                            text: overview_name
                            color: colorText
                            font.pointSize: 18 * ctx.scaleFactor
                            Layout.alignment: Qt.AlignTop
                        }

                        Components.PlainText {
                            Layout.leftMargin: 6
                            text: {
                                if(message !== "") datestr + " " + hourstr;
                                else "";
                            }
                            color: colorMutedText
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
                        color: colorSecondaryText
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
                        source: {
                            if (channel != "") {
                                ctx.ossoIconLookup("general_conference_avatar.png")
                            } else {
                                ctx.ossoIconLookup("general_default_avatar.png")
                            }
                        }
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
