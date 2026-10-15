import QtQuick 2.15
import QtQuick.Controls 2.15
import RealSense.Ui 1.0
import "../../components"
import "../../theme"

AbstractButton {
    id: root

    property string title
    property quaternion targetOrientation
    property bool isCaptured: false
    property bool isDetected: false
    property real progress: 0

    hoverEnabled: true
    enabled: isCaptured
    Accessible.role: Accessible.Button
    Accessible.name: isCaptured ? qsTr("%1: kaydedildi, tekrar kaydetmek için tıklayın").arg(title)
                                : qsTr("%1: bekleniyor").arg(title)

    readonly property color statusColor: isCaptured ? Theme.accent : (isDetected ? Theme.warning : Theme.textMuted)

    ToolTip.visible: hovered && isCaptured
    ToolTip.delay: 400
    ToolTip.text: qsTr("Tekrar kaydet")

    background: Rectangle {
        radius: Theme.radiusMd
        color: root.isDetected ? Theme.surfaceRaised : Theme.background
        border.width: root.isDetected || root.visualFocus ? 2 : 1
        border.color: root.isDetected || root.visualFocus ? root.statusColor : Theme.borderSubtle

        Behavior on color {
            ColorAnimation { duration: Theme.animationFast }
        }
    }

    contentItem: Item {
        OrientationView3D {
            id: poseIllustration
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: tileFooter.top
            orientation: root.targetOrientation
            bodyColor: "#7C8AA0"
            frontColor: "#151C2C"
            accentColor: root.isCaptured ? Theme.accent : Theme.textSecondary
            gridColor: Theme.borderSubtle
            labelColor: "transparent"
        }

        Row {
            id: tileFooter
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: Theme.spacingSm
            spacing: Theme.spacingXs

            SvgIcon {
                anchors.verticalCenter: parent.verticalCenter
                visible: root.isCaptured
                source: root.hovered ? "qrc:/resources/icons/refresh-cw.svg" : "qrc:/resources/icons/check.svg"
                color: Theme.accent
                size: 14
            }

            Text {
                text: root.title
                color: root.isCaptured || root.isDetected ? Theme.textPrimary : Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
                font.weight: Font.Medium
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            height: 3
            radius: 1.5
            width: parent.width * root.progress
            color: Theme.warning
            visible: root.isDetected && root.progress > 0
        }
    }
}
