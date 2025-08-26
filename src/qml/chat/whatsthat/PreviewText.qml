import QtQuick 2.15
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0
import QtGraphicalEffects 1.15

import Conversations 1.0

import "../components" as Components
import "."

// text
RowLayout {
    id: root
    property var title: ""
    property var subtitle: ""
    spacing: 0

    Rectangle {
        color: "#1f2528"
        visible: itemType !== 2
        Layout.fillHeight: true
        Layout.preferredWidth: 42

        Image {
            id: resultIcon
            width: 32
            height: 32
            anchors.centerIn: parent
            source: icon
            sourceSize: Qt.size(width, height)
            cache: true
        }

        MouseArea {
            anchors.fill: parent
            onClicked: preview.itemClicked(index, Qt.point(mouse.x, mouse.y));
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        anchors.margins: 2
        spacing: 0

        Rectangle {
            color: "#1f2528"
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true

            /* @type {Components} */ // clion parser fix
            Components.PlainText {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: undefined

                color: "white"
                text: root.title
                font.pointSize: 14 * ctx.scaleFactor
                Layout.fillWidth: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: preview.itemClicked(index, Qt.point(mouse.x, mouse.y));
            }
        }

        Rectangle {
            color: "#1f2528"
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true

            Components.PlainText {
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: undefined

                color: "white"
                text: root.subtitle
                font.pointSize: 12 * ctx.scaleFactor
                Layout.fillWidth: true
            }

            MouseArea {
                anchors.fill: parent
                onClicked: preview.itemClicked(index, Qt.point(mouse.x, mouse.y));
            }
        }
    }
}