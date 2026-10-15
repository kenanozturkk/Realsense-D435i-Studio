import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../theme"

ColumnLayout {
    id: root

    property string title
    property alias model: settingComboBox.model
    property int currentIndex: 0

    signal activated(int index)

    spacing: Theme.spacingXs

    Text {
        text: root.title
        color: Theme.textSecondary
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeCaption
    }

    ComboBox {
        id: settingComboBox
        Layout.fillWidth: true
        Accessible.name: root.title
        currentIndex: root.currentIndex
        onActivated: {
            root.activated(index)
            currentIndex = Qt.binding(function () { return root.currentIndex })
        }
    }
}
