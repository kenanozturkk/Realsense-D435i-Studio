import QtQuick 2.15
import "../theme"

Rectangle {
    id: root

    property string text
    property color statusColor: Theme.textMuted
    property bool isPulsing: false

    implicitWidth: badgeRow.implicitWidth + 2 * Theme.spacingMd
    implicitHeight: 28
    radius: height / 2
    color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.12)
    border.color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.35)
    Accessible.role: Accessible.StaticText
    Accessible.name: text

    Row {
        id: badgeRow
        anchors.centerIn: parent
        spacing: Theme.spacingSm

        Rectangle {
            id: statusDot
            width: 8
            height: 8
            radius: 4
            anchors.verticalCenter: parent.verticalCenter
            color: root.statusColor

            SequentialAnimation on opacity {
                running: root.isPulsing && root.visible
                loops: Animation.Infinite
                alwaysRunToEnd: true
                OpacityAnimator { from: 1.0; to: 0.3; duration: 600; easing.type: Easing.InOutQuad }
                OpacityAnimator { from: 0.3; to: 1.0; duration: 600; easing.type: Easing.InOutQuad }
            }
        }

        Text {
            text: root.text
            color: root.statusColor
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
            font.weight: Font.Medium
        }
    }
}
