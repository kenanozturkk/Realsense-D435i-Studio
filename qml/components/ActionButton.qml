import QtQuick 2.15
import QtQuick.Controls 2.15
import "../theme"

Button {
    id: root

    property url iconSource
    property bool isPrimary: false
    property string toolTipText

    implicitHeight: Theme.controlHeight
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0
    leftPadding: Theme.spacingLg
    rightPadding: Theme.spacingLg
    hoverEnabled: true
    Accessible.name: text

    ToolTip.visible: hovered && toolTipText.length > 0
    ToolTip.delay: 500
    ToolTip.text: toolTipText

    readonly property color foregroundColor: {
        if (!enabled)
            return Theme.textMuted
        return isPrimary ? Theme.accentForeground : Theme.textPrimary
    }

    background: Rectangle {
        radius: Theme.radiusMd
        color: {
            if (root.isPrimary)
                return root.down ? Qt.darker(Theme.accent, 1.15) : (root.hovered ? Qt.lighter(Theme.accent, 1.1) : Theme.accent)
            return root.down ? Theme.surfaceRaised : (root.hovered ? Theme.surfaceHover : Theme.surface)
        }
        border.color: root.visualFocus ? Theme.accent : (root.isPrimary ? "transparent" : Theme.borderSubtle)
        border.width: root.visualFocus ? 2 : 1
        opacity: root.enabled ? 1.0 : 0.5

        Behavior on color {
            ColorAnimation { duration: Theme.animationFast }
        }
    }

    contentItem: Row {
        spacing: Theme.spacingSm

        SvgIcon {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.iconSource.toString().length > 0
            source: root.iconSource
            color: root.foregroundColor
            size: 16
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: root.foregroundColor
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            font.weight: Font.Medium
        }
    }
}
