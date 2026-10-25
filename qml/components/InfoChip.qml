import QtQuick 2.15
import "../theme"

Rectangle {
    id: root

    property string label
    property string value

    implicitWidth: chipRow.implicitWidth + 2 * Theme.spacingMd
    implicitHeight: 26
    radius: height / 2
    color: Theme.surface
    border.color: Theme.borderSubtle

    Row {
        id: chipRow
        anchors.centerIn: parent
        spacing: Theme.spacingXs

        Text {
            text: root.label
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
        }

        Text {
            text: root.value
            color: Theme.textPrimary
            font.family: Theme.monoFontFamily
            font.pixelSize: Theme.fontSizeCaption
        }
    }
}
