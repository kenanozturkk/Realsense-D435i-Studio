import QtQuick 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import RealSense.Ui 1.0
import "../components"
import "../theme"
import "depth"

Item {
    id: root

    property bool isPausedByUser: false

    onVisibleChanged: {
        if (visible && !isPausedByUser && !RecordingController.isRecording)
            DepthCameraController.start()
        else if (!visible)
            DepthCameraController.stop()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXl
        spacing: Theme.spacingLg

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMd

            Column {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: qsTr("Derinlik ve Renk")
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeTitle
                    font.weight: Font.DemiBold
                }

                Text {
                    text: qsTr("Fareyi görüntü üzerinde gezdirerek o noktanın mesafesini ölçün")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBody
                }
            }

            StatusBadge {
                text: DepthCameraController.isStreaming ? qsTr("Akış açık") : qsTr("Durduruldu")
                statusColor: DepthCameraController.isStreaming ? Theme.accent : Theme.textMuted
            }

            ActionButton {
                text: DepthCameraController.isStreaming ? qsTr("Duraklat") : qsTr("Başlat")
                iconSource: DepthCameraController.isStreaming ? "qrc:/resources/icons/square.svg" : "qrc:/resources/icons/play.svg"
                isPrimary: !DepthCameraController.isStreaming
                enabled: DeviceManager.isConnected && !RecordingController.isRecording
                onClicked: {
                    root.isPausedByUser = DepthCameraController.isStreaming
                    if (DepthCameraController.isStreaming)
                        DepthCameraController.stop()
                    else
                        DepthCameraController.start()
                }
            }
        }

        Row {
            spacing: Theme.spacingSm

            InfoChip {
                label: qsTr("Merkez")
                value: DepthCameraController.centerDistance > 0 ? qsTr("%1 m").arg(DepthCameraController.centerDistance.toFixed(3)) : "—"
            }

            InfoChip {
                label: qsTr("Geçerli piksel")
                value: qsTr("%1%").arg(Math.round(DepthCameraController.validPixelRatio * 100))
            }

            InfoChip {
                label: qsTr("Derinlik")
                value: qsTr("%1 fps").arg(DepthCameraController.depthFrameRate)
            }

            InfoChip {
                label: qsTr("Renk")
                value: qsTr("%1 fps").arg(DepthCameraController.colorFrameRate)
            }

            InfoChip {
                label: qsTr("Derinlik çözünürlüğü")
                value: DepthCameraController.depthResolution.width > 0
                       ? qsTr("%1 × %2").arg(DepthCameraController.depthResolution.width).arg(DepthCameraController.depthResolution.height)
                       : "—"
            }
        }

        Text {
            visible: DepthCameraController.errorMessage.length > 0
            Layout.fillWidth: true
            text: DepthCameraController.errorMessage
            color: Theme.danger
            wrapMode: Text.WordWrap
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingLg

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spacingLg

                StreamCard {
                    title: qsTr("Derinlik")
                    subtitle: qsTr("renk = uzaklık")
                    stream: FrameSource.Depth
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                StreamCard {
                    title: qsTr("Renk")
                    subtitle: DepthCameraController.isAlignedToColor ? qsTr("hizalı · ölçüm yapılabilir")
                                                                     : qsTr("hizalama kapalı")
                    stream: FrameSource.Color
                    canMeasure: DepthCameraController.isAlignedToColor
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            DepthSettingsPanel {
                Layout.fillWidth: false
                Layout.preferredWidth: 340
                Layout.fillHeight: true
            }
        }
    }
}
