import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0
import MaemoConfig 1.0

import "../components" as Components

Rectangle {
    id: appWindow
    visible: true
    color: "black"

    signal overviewRowClicked(string group_uid, string local_uid, string remote_uid, string event_id, string service_id);

    property Overview overviewWindow: Overview {
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    property Search searchWindow: Search {
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    Components.StackViewFancy {
        id: view
        anchors.fill: parent
        clip: true

        initialItem: overviewWindow
    }

    Connections {
        target: searchWindow
        onItemClicked: {
            overviewRowClicked(group_uid, local_uid, remote_uid, event_id, service_id);
        }
    }

    Connections {
        target: mainWindow
        onRequestOverviewSearchWindow: {
            view.stackReplace(searchWindow, "", 0);
        }
    }
}
