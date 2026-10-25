import QtQuick 2.15
import QtQuick.Controls 2.15
import "../theme"

AbstractButton {
    id: root

    property url iconSource

    checkable: true
    implicitWidth: Theme.navigationRailWidth - 2 * Theme.spacingSm
    implicitHeight: 64
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.PageTab
    Accessible.name: text

    readonly property color foregroundColor: {
        if (!enabled)
            return Theme.textMuted
        return checked ? Theme.accent : (hovered ? Theme.textPrimary : Theme.textSecondary)
    }

    background: Rectangle {
        radius: Theme.radiusMd
        color: root.checked ? Theme.surfaceRaised : (root.hovered ? Theme.surface : "transparent")
        border.width: root.visualFocus ? 2 : 0
        border.color: Theme.accent

        Rectangle {
            width: 3
            height: parent.height * 0.5
            radius: 2
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.accent
            visible: root.checked
        }

        Behavior on color {
            ColorAnimation { duration: Theme.animationFast }
        }
    }

    contentItem: Column {
        spacing: Theme.spacingXs
        topPadding: Theme.spacingSm

        SvgIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            source: root.iconSource
            color: root.foregroundColor
            size: 22
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: root.text
            color: root.foregroundColor
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
            font.weight: root.checked ? Font.Medium : Font.Normal
            elide: Text.ElideRight
        }
    }
}
