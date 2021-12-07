import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

Item {
    id: root
    width: 76
    height: 76

    signal clicked();

    Rectangle {  // lies: it is a circle
        anchors.fill: parent
        color: "white"
        opacity: 0.3
        radius: width * 0.5
    }

    Image {
        width: 48
        height: 48
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        source: ctx.ossoIconLookup("rss_reader_move_down.png")
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.clicked();
        }
    }
}
