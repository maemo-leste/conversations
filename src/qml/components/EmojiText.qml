import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Text {
    property string plainText: ""
    property bool _emoji: colorEmojiEnabled && ctx.hasEmoji(plainText)

    textFormat: _emoji ? Text.RichText : Text.PlainText
    text: _emoji ? ctx.emojify(plainText) : plainText
    font.hintingPreference: Font.PreferNoHinting
}
