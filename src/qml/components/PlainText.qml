import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.0

import "." as Components

// Ensure Text components use `Text.PlainText` by
// default, for security reasons.

Text {
    textFormat: Text.PlainText
}
