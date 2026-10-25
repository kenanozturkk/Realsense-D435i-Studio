import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../theme"

Rectangle {
    id: root

    property string label
    property string description
    property real valueDegrees: 0
    property color accentColor: Theme.accent
    readonly property real displayedValue: Math.round(valueDegrees * 10) / 10

    implicitHeight: 72
    radius: Theme.radiusLg
    color: Theme.surface
    border.color: Theme.borderSubtle
    Accessible.role: Accessible.StaticText
    Accessible.name: qsTr("%1: %2 derece").arg(label).arg(valueDegrees.toFixed(1))

    Rectangle {
        width: 4
        radius: 2
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spacingMd
        color: root.accentColor
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingXl
        anchors.rightMargin: Theme.spacingLg
        spacing: Theme.spacingMd

        Column {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: root.label
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                font.weight: Font.Medium
            }

            Text {
                text: root.description
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }
        }

        Text {
            text: (root.displayedValue >= 0 ? "+" : "") + root.displayedValue.toFixed(1) + "°"
            color: root.accentColor
            font.family: Theme.monoFontFamily
            font.pixelSize: Theme.fontSizeReadout
            font.weight: Font.Medium
            horizontalAlignment: Text.AlignRight
            Layout.preferredWidth: 150
        }
    }
}
