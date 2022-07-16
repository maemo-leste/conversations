import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "black"
    anchors.fill: parent
    property int itemHeight: 76;
    property string highlight: "#00a2ff"

    ListModel {
        id: xxx2
        ListElement {
            name: "Test"
        }
        ListElement {
            name: "Bla"
        }
        ListElement {
            name: "Foo"
        }
        ListElement {
            name: "Bar"
        }
    }

    ListModel {
        id: xxx
        ListElement {
            name: "Bill Smith"
            datestr: "10/22/2021 | 11:04 pm"
            type: "call"
            message: ""
            status: "sent"
        }
        ListElement {
            datestr: "10/22/2021 | 11:04 pm"
            name: "John Brown"
            type: "call"
            message: "received"
        }
        ListElement {
            datestr: "10/22/2021 | 11:04 pm"
            name: "Sam Wise"
            type: "sms"
            message: "I'm going back, are you done or drinking still?"
            status: "sent"
        }
        ListElement {
            datestr: "10/22/2021 | 11:04 pm"
            name: "MijnOverheid"
            type: "sms"
            message: "Uw DigiD sms-code om in te loggen <snip>"
            status: "read"
        }
        ListElement {
            datestr: "10/22/2021 | 11:04 pm"
            name: "Wizzup"
            type: "call"
            message: ""
            status: "missed"
        }
        ListElement {
            datestr: "10/22/2021 | 11:04 pm"
            name: "dsc"
            type: "sms"
            message: "Call me later plz."
            status: "unread"
        }
        ListElement {
            datestr: "10/22/2021 | 11:04 pm"
            name: "Bar Foo"
            type: "call"
            message: ""
            status: "sent"
        }
    }

    property int topBarHeight: 54

    ListView {
        height: root.topBarHeight
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        model: xxx2
        orientation: Qt.Horizontal
        layoutDirection: Qt.LeftToRight
        delegate: Rectangle {
            //color: index % 2 == 0 ? "grey" : "green"
            width: text.implicitWidth + 80
            height: root.topBarHeight
            color: "green"

            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width * 0.8
                height: parent.height * 0.7
                spacing: 0

                Rectangle {
                    Layout.preferredHeight: parent.height * 0.8
                    Layout.fillWidth: true
                    color: "red"
                    radius: 4

                    Text {
                        id: text
                        text: name
                        font.pointSize: 18
                        color: "white"
                        anchors.centerIn: parent
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 4
                    Layout.fillWidth: true
                    color: "transparent"
                    visible: index == 0

                    Image {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        horizontalAlignment: Image.AlignLeft

                        height: parent.preferredHeight
                        source: "https://i.imgur.com/4SW610Y.png"
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
                    console.log('xex');
                }
            }
        }
    }

    ListView {
        anchors.topMargin: root.topBarHeight
        anchors.fill: parent

        model: xxx
        delegate: Rectangle {
            height: itemHeight
            width: parent.width
            color: "black"

            Item {
                height: itemHeight
                width: parent.width

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true

                    onEntered: {
                        parent.parent.color = highlight;
                    }
                    onExited: {
                        parent.parent.color = "black";
                    }
                    onClicked: {
                        console.log('e');
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
                        source: "https://live.staticflickr.com/2709/4163641325_8f60d4f75a_m.jpg"
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

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: itemHeight / 2

                        Text {
                            text: name
                            color: "white"
                            font.pointSize: 18
                            Layout.alignment: Qt.AlignTop
                        }

                        Text {
                            text: {
                                if(message !== "") datestr;
                                else "";
                            }
                            color: "grey"
                            font.pointSize: 12
                            Layout.alignment: Qt.AlignTop

                        }

                        Item {
                            Layout.fillWidth: true
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.preferredHeight: itemHeight / 2
                        color: "grey"
                        font.pointSize: 14
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
                        source: "https://live.staticflickr.com/2709/4163641325_8f60d4f75a_m.jpg"
                        smooth: true
                        visible: true
                    }
                }

            }
        }
    }

}
