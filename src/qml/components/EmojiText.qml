import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Text {
    property string plainText: ""

    textFormat: Text.RichText
    text: ctx.emojify(plainText)
    font.hintingPreference: Font.PreferNoHinting

    FontLoader {
        source: colorEmojiEnabled ? colorEmojiFontUrl : ""
    }
}
