import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"

RowLayout {
    id: root

    property string title
    property string description
    property bool checked: false

    signal toggled(bool isChecked)

    spacing: Theme.spacingMd

    Column {
        Layout.fillWidth: true
        spacing: 2

        Text {
            width: parent.width
            text: root.title
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            visible: root.description.length > 0
            text: root.description
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
            wrapMode: Text.WordWrap
        }
    }

    Switch {
        id: settingSwitch
        Accessible.name: root.title
        checked: root.checked
        onToggled: {
            root.toggled(checked)
            checked = Qt.binding(function () { return root.checked })
        }
    }
}
