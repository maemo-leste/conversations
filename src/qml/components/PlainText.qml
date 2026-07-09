import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "." as Components

// Ensure Text components use `Text.PlainText` by
// default, for security reasons.

Text {
    textFormat: Text.PlainText
    font.hintingPreference: Font.PreferNoHinting

    FontLoader {
        id: notoEmoji
        source: colorEmojiEnabled ? colorEmojiFontUrl : ""
    }

    font.family: colorEmojiEnabled && notoEmoji.status === FontLoader.Ready ? notoEmoji.name : ""
}
