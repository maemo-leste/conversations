import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "." as Components

// Ensure Text components use `Text.PlainText` by
// default, for security reasons.

Text {
    textFormat: Text.PlainText
}
