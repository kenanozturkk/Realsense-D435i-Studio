import QtQuick 2.15
import QtQuick.Controls 2.15
import RealSense.Core 1.0
import "../../theme"

Rectangle {
    id: root

    width: overlayColumn.implicitWidth + 2 * Theme.spacingXl
    height: overlayColumn.implicitHeight + 2 * Theme.spacingLg
    radius: Theme.radiusMd
    color: Qt.rgba(0.06, 0.09, 0.16, 0.92)
    border.color: Theme.warning
    visible: ImuController.trackingState === ImuController.Calibrating

    Column {
        id: overlayColumn
        anchors.centerIn: parent
        spacing: Theme.spacingSm

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: ImuController.isStationary ? qsTr("Gyro sıfır noktası öğreniliyor…")
                                             : qsTr("Kamerayı sabit bir yüzeye bırakın")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            font.weight: Font.Medium
        }

        ProgressBar {
            id: calibrationProgress
            anchors.horizontalCenter: parent.horizontalCenter
            width: 260
            value: ImuController.biasCalibrationProgress

            background: Rectangle {
                implicitHeight: 6
                radius: 3
                color: Theme.surfaceRaised
            }

            contentItem: Item {
                Rectangle {
                    width: calibrationProgress.visualPosition * parent.width
                    height: parent.height
                    radius: 3
                    color: Theme.warning
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("%1%").arg(Math.round(ImuController.biasCalibrationProgress * 100))
            color: Theme.textSecondary
            font.family: Theme.monoFontFamily
            font.pixelSize: Theme.fontSizeCaption
        }
    }
}
