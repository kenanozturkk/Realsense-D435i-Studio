import QtQuick 2.15
import "../theme"

Row {
    id: root

    property var entries: []

    spacing: Theme.spacingLg

    Repeater {
        model: root.entries

        delegate: Row {
            required property var modelData

            spacing: Theme.spacingXs

            Rectangle {
                width: 10
                height: 3
                radius: 1.5
                anchors.verticalCenter: parent.verticalCenter
                color: modelData.color
            }

            Text {
                text: modelData.name
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }
        }
    }
}
