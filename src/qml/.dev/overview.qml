import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.15

Rectangle {
    color: "black"
    anchors.fill: parent
    property int itemHeight: 76;
    property string highlight: "#00a2ff"

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


    ListView {
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
