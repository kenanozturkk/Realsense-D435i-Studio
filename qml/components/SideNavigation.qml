import QtQuick 2.15
import QtQuick.Controls 2.15
import "../theme"

Rectangle {
    id: root

    property alias model: navigationRepeater.model
    property int currentIndex: 0

    implicitWidth: Theme.navigationRailWidth
    color: Theme.background

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.borderSubtle
    }

    ButtonGroup {
        id: navigationGroup
        exclusive: true
    }

    Column {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: Theme.spacingLg
        spacing: Theme.spacingSm

        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/resources/icons/scan-line.svg"
            sourceSize: Qt.size(28, 28)
            opacity: 0.9
            Accessible.ignored: true
        }

        Item {
            width: 1
            height: Theme.spacingMd
        }

        Repeater {
            id: navigationRepeater

            delegate: NavigationButton {
                required property int index
                required property string title
                required property string iconPath

                text: title
                iconSource: iconPath
                checked: index === root.currentIndex
                ButtonGroup.group: navigationGroup
                onClicked: root.currentIndex = index
            }
        }
    }
}
