import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../../theme"

GridLayout {
    id: root

    property vector3d offset: Qt.vector3d(0, 0, 0)
    property vector3d scale: Qt.vector3d(1, 1, 1)

    readonly property var axisRows: [
        { name: qsTr("İleri (X)"), offset: offset.x, scale: scale.x, color: Theme.axisXColor },
        { name: qsTr("Sol (Y)"), offset: offset.y, scale: scale.y, color: Theme.axisYColor },
        { name: qsTr("Yukarı (Z)"), offset: offset.z, scale: scale.z, color: Theme.axisZColor }
    ]

    columns: 3
    columnSpacing: Theme.spacingLg
    rowSpacing: Theme.spacingSm

    Text {
        text: qsTr("Eksen")
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeCaption
        Layout.fillWidth: true
    }

    Text {
        text: qsTr("Ofset (m/s²)")
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeCaption
        horizontalAlignment: Text.AlignRight
        Layout.preferredWidth: 110
    }

    Text {
        text: qsTr("Ölçek")
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeCaption
        horizontalAlignment: Text.AlignRight
        Layout.preferredWidth: 90
    }

    Repeater {
        model: root.axisRows

        delegate: Text {
            required property var modelData
            required property int index

            Layout.row: index + 1
            Layout.column: 0
            Layout.fillWidth: true
            text: modelData.name
            color: modelData.color
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
        }
    }

    Repeater {
        model: root.axisRows

        delegate: Text {
            required property var modelData
            required property int index

            Layout.row: index + 1
            Layout.column: 1
            Layout.preferredWidth: 110
            horizontalAlignment: Text.AlignRight
            text: (modelData.offset >= 0 ? "+" : "") + modelData.offset.toFixed(4)
            color: Theme.textPrimary
            font.family: Theme.monoFontFamily
            font.pixelSize: Theme.fontSizeBody
        }
    }

    Repeater {
        model: root.axisRows

        delegate: Text {
            required property var modelData
            required property int index

            Layout.row: index + 1
            Layout.column: 2
            Layout.preferredWidth: 90
            horizontalAlignment: Text.AlignRight
            text: modelData.scale.toFixed(5)
            color: Theme.textPrimary
            font.family: Theme.monoFontFamily
            font.pixelSize: Theme.fontSizeBody
        }
    }
}
