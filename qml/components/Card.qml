import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

Rectangle {
    id: root

    property string title
    property string subtitle
    property real contentImplicitHeight: 0
    default property alias content: contentArea.data

    implicitHeight: cardLayout.implicitHeight + 2 * Theme.spacingLg
    color: Theme.surface
    radius: Theme.radiusLg
    border.color: Theme.borderSubtle

    ColumnLayout {
        id: cardLayout
        anchors.fill: parent
        anchors.margins: Theme.spacingLg
        spacing: Theme.spacingMd

        RowLayout {
            visible: root.title.length > 0
            spacing: Theme.spacingSm
            Layout.fillWidth: true

            Text {
                text: root.title
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                font.weight: Font.Medium
            }

            Text {
                text: root.subtitle
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }

        Item {
            id: contentArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root.contentImplicitHeight
        }
    }
}
