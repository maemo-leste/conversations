import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Text {
    property string plainText: ""

    text: plainText
    textFormat: Text.PlainText
    font.hintingPreference: Font.PreferNoHinting
}
