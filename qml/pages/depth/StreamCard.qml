import QtQuick 2.15
import RealSense.Core 1.0
import RealSense.Ui 1.0
import "../../components"
import "../../theme"

Card {
    id: root

    property int stream: FrameSource.Depth
    property bool canMeasure: true
    property real hoverDistance: 0

    readonly property bool isMeasuring: canMeasure && frameView.isHovering
    readonly property point cursorPoint: Qt.point(frameView.imageRect.x + frameView.hoverPosition.x * frameView.imageRect.width,
                                                  frameView.imageRect.y + frameView.hoverPosition.y * frameView.imageRect.height)

    function refreshHoverDistance() {
        hoverDistance = isMeasuring ? DepthCameraController.distanceAt(frameView.hoverPosition) : 0
    }

    Connections {
        target: DepthCameraController
        enabled: root.isMeasuring

        function onFramesUpdated() {
            root.refreshHoverDistance()
        }
    }

    VideoFrameView {
        id: frameView
        anchors.fill: parent
        source: DepthCameraController
        stream: root.stream
        onHoverChanged: root.refreshHoverDistance()
    }

    Text {
        anchors.centerIn: parent
        visible: !frameView.hasImage
        text: DeviceManager.isConnected ? qsTr("Akış bekleniyor…") : qsTr("Kamera bağlı değil")
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeBody
    }

    Item {
        anchors.fill: parent
        visible: root.isMeasuring
        Accessible.ignored: true

        Rectangle {
            x: frameView.imageRect.x
            y: root.cursorPoint.y
            width: frameView.imageRect.width
            height: 1
            color: Qt.rgba(1, 1, 1, 0.45)
        }

        Rectangle {
            x: root.cursorPoint.x
            y: frameView.imageRect.y
            width: 1
            height: frameView.imageRect.height
            color: Qt.rgba(1, 1, 1, 0.45)
        }

        Rectangle {
            id: distanceBubble

            readonly property bool fitsOnRight: root.cursorPoint.x + width + 16 < frameView.imageRect.right

            x: fitsOnRight ? root.cursorPoint.x + 12 : root.cursorPoint.x - width - 12
            y: Math.max(frameView.imageRect.y, root.cursorPoint.y - height - 12)
            width: distanceText.implicitWidth + 2 * Theme.spacingMd
            height: distanceText.implicitHeight + Theme.spacingSm
            radius: Theme.radiusSm
            color: Qt.rgba(0.06, 0.09, 0.16, 0.9)
            border.color: root.hoverDistance > 0 ? Theme.accent : Theme.border

            Text {
                id: distanceText
                anchors.centerIn: parent
                text: root.hoverDistance > 0 ? qsTr("%1 m").arg(root.hoverDistance.toFixed(3)) : qsTr("ölçüm yok")
                color: root.hoverDistance > 0 ? Theme.textPrimary : Theme.textMuted
                font.family: Theme.monoFontFamily
                font.pixelSize: Theme.fontSizeBody
                font.weight: Font.Medium
            }
        }
    }
}
