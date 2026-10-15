import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import RealSense.Ui 1.0
import "../../components"
import "../../theme"

RowLayout {
    id: root

    readonly property int suggestedIndex: CalibrationController.suggestedPoseIndex
    readonly property var suggestedPose: suggestedIndex >= 0 ? CalibrationController.poses[suggestedIndex] : null
    readonly property bool isSuggestedPoseDetected: suggestedIndex >= 0
                                                    && suggestedIndex === CalibrationController.detectedPoseIndex

    readonly property string captureStatusText: {
        if (!isSuggestedPoseDetected)
            return qsTr("Kamerayı bu duruşa getirin")
        if (!CalibrationController.isDeviceStill)
            return qsTr("Duruş algılandı · kamerayı bırakın, kıpırdamasın")
        return qsTr("Kaydediliyor… %1%").arg(Math.round(CalibrationController.captureProgress * 100))
    }

    spacing: Theme.spacingLg

    Card {
        title: qsTr("Duruş %1 / 6").arg(Math.min(CalibrationController.capturedPoseCount + 1, 6))
        subtitle: root.suggestedPose ? root.suggestedPose.title : ""
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacingMd

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: root.suggestedPose ? root.suggestedPose.instruction : ""
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSubtitle
            }

            OrientationView3D {
                Layout.fillWidth: true
                Layout.fillHeight: true
                orientation: root.suggestedPose ? root.suggestedPose.targetOrientation : Qt.quaternion(1, 0, 0, 0)
                bodyColor: "#7C8AA0"
                frontColor: "#151C2C"
                accentColor: Theme.accent
                gridColor: Theme.border
                labelColor: "transparent"
            }

            Text {
                text: root.captureStatusText
                color: root.isSuggestedPoseDetected ? Theme.warning : Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                font.weight: Font.Medium
            }

            ProgressBar {
                id: captureProgressBar
                Layout.fillWidth: true
                value: root.isSuggestedPoseDetected ? CalibrationController.captureProgress : 0

                background: Rectangle {
                    implicitHeight: 6
                    radius: 3
                    color: Theme.surfaceRaised
                }

                contentItem: Item {
                    Rectangle {
                        width: captureProgressBar.visualPosition * parent.width
                        height: parent.height
                        radius: 3
                        color: Theme.warning
                    }
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: false
        Layout.preferredWidth: 440
        Layout.fillHeight: true
        spacing: Theme.spacingMd

        Text {
            text: qsTr("Kaydedilen duruşlar: %1 / 6").arg(CalibrationController.capturedPoseCount)
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
        }

        GridLayout {
            columns: 2
            rowSpacing: Theme.spacingMd
            columnSpacing: Theme.spacingMd
            Layout.fillWidth: true
            Layout.fillHeight: true

            Repeater {
                model: CalibrationController.poses

                delegate: PoseTile {
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    title: modelData.title
                    targetOrientation: modelData.targetOrientation
                    isCaptured: modelData.isCaptured
                    isDetected: index === CalibrationController.detectedPoseIndex
                    progress: isDetected ? CalibrationController.captureProgress : 0
                    onClicked: CalibrationController.recapturePose(index)
                }
            }
        }

        ActionButton {
            text: qsTr("İptal")
            iconSource: "qrc:/resources/icons/x.svg"
            Layout.alignment: Qt.AlignRight
            onClicked: CalibrationController.cancelWizard()
        }
    }
}
