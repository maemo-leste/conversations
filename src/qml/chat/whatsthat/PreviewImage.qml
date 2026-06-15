import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Conversations 1.0

import "../components" as Components
import "."

// image
Rectangle {
    property int previewItemType: 0
    property var previewFilePath: ""
    color: "transparent"

    Image {
        anchors.fill: parent
        source: "image://previewImage/" + previewFilePath
        fillMode: Image.PreserveAspectCrop

        Rectangle {
            color: "#90000000"
            width: 40
            height: 40
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.bottomMargin: 10

            Image {
                anchors.centerIn: parent
                source: "qrc:/Zoom.svg"
                sourceSize: Qt.size(parent.width - 8, parent.height - 8)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: preview.itemClicked(index, Qt.point(mouse.x, mouse.y));
    }
}