import QtQuick 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import RealSense.Ui 1.0
import "../../components"
import "../../theme"

RowLayout {
    id: root

    signal playRequested(string filePath)

    function formatDuration(totalSeconds) {
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = Math.floor(totalSeconds % 60)
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    spacing: Theme.spacingLg

    ColumnLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: Theme.spacingLg

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingLg

            FramePreviewCard {
                title: qsTr("Derinlik")
                frameSource: RecordingController
                stream: FrameSource.Depth
                placeholderText: qsTr("Önizleme kayıt sırasında görünür")
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            FramePreviewCard {
                title: qsTr("Renk")
                frameSource: RecordingController
                stream: FrameSource.Color
                placeholderText: qsTr("Önizleme kayıt sırasında görünür")
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        Card {
            Layout.fillWidth: true
            contentImplicitHeight: statusRow.implicitHeight

            RowLayout {
                id: statusRow
                anchors.fill: parent
                spacing: Theme.spacingXl

                StatusBadge {
                    text: RecordingController.isRecording ? qsTr("KAYIT") : qsTr("Hazır")
                    statusColor: RecordingController.isRecording ? Theme.danger : Theme.textMuted
                    isPulsing: RecordingController.isRecording
                }

                MetricRow {
                    Layout.preferredWidth: 140
                    label: qsTr("Süre")
                    value: root.formatDuration(RecordingController.elapsedSeconds)
                }

                MetricRow {
                    Layout.preferredWidth: 220
                    label: qsTr("Dosya boyutu")
                    value: qsTr("%1 MB").arg(RecordingController.fileSizeMegabytes.toFixed(1))
                }

                MetricRow {
                    Layout.preferredWidth: 200
                    visible: RecordingController.recordsDepth
                    label: qsTr("Derinlik karesi")
                    value: RecordingController.recordedDepthFrames
                    valueColor: RecordingController.isRecording && RecordingController.recordedDepthFrames === 0
                                ? Theme.warning : Theme.textPrimary
                }

                MetricRow {
                    Layout.preferredWidth: 180
                    visible: RecordingController.recordsColor
                    label: qsTr("Renk karesi")
                    value: RecordingController.recordedColorFrames
                    valueColor: RecordingController.isRecording && RecordingController.recordedColorFrames === 0
                                ? Theme.warning : Theme.textPrimary
                }

                Item {
                    Layout.fillWidth: true
                }

                Text {
                    visible: RecordingController.errorMessage.length > 0
                    text: RecordingController.errorMessage
                    color: Theme.danger
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBody
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: false
        Layout.preferredWidth: 360
        Layout.fillHeight: true
        spacing: Theme.spacingLg

        Card {
            title: qsTr("Yeni kayıt")
            subtitle: qsTr("848 × 480 · 30 fps")
            contentImplicitHeight: recordSettings.implicitHeight
            Layout.fillWidth: true

            ColumnLayout {
                id: recordSettings
                anchors.fill: parent
                spacing: Theme.spacingSm

                SettingSwitch {
                    Layout.fillWidth: true
                    enabled: !RecordingController.isRecording
                    title: qsTr("Derinlik")
                    checked: RecordingController.recordsDepth
                    onToggled: RecordingController.recordsDepth = isChecked
                }

                SettingSwitch {
                    Layout.fillWidth: true
                    enabled: !RecordingController.isRecording
                    title: qsTr("Renk")
                    description: qsTr("Sıkıştırılmamış; dosya boyutunu en çok bu artırır.")
                    checked: RecordingController.recordsColor
                    onToggled: RecordingController.recordsColor = isChecked
                }

                SettingSwitch {
                    Layout.fillWidth: true
                    enabled: !RecordingController.isRecording
                    title: qsTr("IMU")
                    description: qsTr("Oynatırken yönelim de hesaplanır.")
                    checked: RecordingController.recordsImu
                    onToggled: RecordingController.recordsImu = isChecked
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: qsTr("Kayıt sırasında canlı IMU ve derinlik ekranları duraklatılır, kayıt bitince IMU kaldığı yerden devam eder.")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeCaption
                }

                ActionButton {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.spacingSm
                    text: RecordingController.isRecording ? qsTr("Kaydı Durdur") : qsTr("Kaydı Başlat")
                    iconSource: RecordingController.isRecording ? "qrc:/resources/icons/square.svg"
                                                                : "qrc:/resources/icons/circle-dot.svg"
                    isPrimary: !RecordingController.isRecording
                    enabled: DeviceManager.isConnected || RecordingController.isRecording
                    onClicked: RecordingController.isRecording ? RecordingController.stopRecording()
                                                               : RecordingController.startRecording()
                }
            }
        }

        RecordingsListCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            onPlayRequested: root.playRequested(filePath)
        }
    }
}
