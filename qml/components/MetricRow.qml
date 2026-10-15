import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

RowLayout {
    id: root

    property string label
    property string value
    property color valueColor: Theme.textPrimary

    spacing: Theme.spacingSm

    Text {
        text: root.label
        color: Theme.textSecondary
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeBody
        Layout.fillWidth: true
        elide: Text.ElideRight
    }

    Text {
        text: root.value
        color: root.valueColor
        font.family: Theme.monoFontFamily
        font.pixelSize: Theme.fontSizeBody
        horizontalAlignment: Text.AlignRight
    }
}
