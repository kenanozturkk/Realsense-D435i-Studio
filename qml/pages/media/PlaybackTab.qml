import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import RealSense.Ui 1.0
import "../../components"
import "../../theme"

RowLayout {
    id: root

    property string loadedFilePath

    function openRecording(filePath) {
        loadedFilePath = filePath
        PlaybackController.open(filePath)
    }

    function formatTime(totalSeconds) {
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
                frameSource: PlaybackController
                stream: FrameSource.Depth
                placeholderText: PlaybackController.isLoaded ? qsTr("Bu kayıtta derinlik yok") : qsTr("Listeden bir kayıt seçin")
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            FramePreviewCard {
                title: qsTr("Renk")
                frameSource: PlaybackController
                stream: FrameSource.Color
                placeholderText: PlaybackController.isLoaded ? qsTr("Bu kayıtta renk yok") : qsTr("Listeden bir kayıt seçin")
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        Card {
            visible: PlaybackController.hasImu
            title: qsTr("Kayıttaki yönelim")
            subtitle: qsTr("roll %1° · pitch %2° · yaw %3°")
                          .arg(PlaybackController.roll.toFixed(1))
                          .arg(PlaybackController.pitch.toFixed(1))
                          .arg(PlaybackController.yaw.toFixed(1))
            Layout.fillWidth: true
            Layout.preferredHeight: 220

            OrientationView3D {
                anchors.fill: parent
                orientation: PlaybackController.orientation
                bodyColor: "#7C8AA0"
                frontColor: "#151C2C"
                accentColor: Theme.accent
                gridColor: Theme.border
                labelColor: Theme.textSecondary
            }
        }

        Card {
            Layout.fillWidth: true
            contentImplicitHeight: transportRow.implicitHeight
            enabled: PlaybackController.isLoaded

            RowLayout {
                id: transportRow
                anchors.fill: parent
                spacing: Theme.spacingMd

                ActionButton {
                    text: PlaybackController.isPlaying ? qsTr("Duraklat") : qsTr("Oynat")
                    iconSource: PlaybackController.isPlaying ? "qrc:/resources/icons/square.svg" : "qrc:/resources/icons/play.svg"
                    isPrimary: !PlaybackController.isPlaying
                    onClicked: PlaybackController.isPlaying ? PlaybackController.pause() : PlaybackController.play()
                }

                Text {
                    text: root.formatTime(PlaybackController.positionSeconds)
                    color: Theme.textPrimary
                    font.family: Theme.monoFontFamily
                    font.pixelSize: Theme.fontSizeBody
                }

                Slider {
                    id: seekSlider
                    Layout.fillWidth: true
                    from: 0
                    to: Math.max(PlaybackController.durationSeconds, 0.001)
                    onMoved: PlaybackController.seek(value)
                    Accessible.name: qsTr("Oynatma konumu")

                    Binding on value {
                        value: PlaybackController.positionSeconds
                        when: !seekSlider.pressed
                    }
                }

                Text {
                    text: root.formatTime(PlaybackController.durationSeconds)
                    color: Theme.textSecondary
                    font.family: Theme.monoFontFamily
                    font.pixelSize: Theme.fontSizeBody
                }

                ActionButton {
                    text: qsTr("Kapat")
                    iconSource: "qrc:/resources/icons/x.svg"
                    onClicked: {
                        PlaybackController.close()
                        root.loadedFilePath = ""
                    }
                }
            }
        }

        Text {
            visible: PlaybackController.errorMessage.length > 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: PlaybackController.errorMessage
            color: Theme.danger
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
        }
    }

    RecordingsListCard {
        Layout.fillWidth: false
        Layout.preferredWidth: 360
        Layout.fillHeight: true
        selectedFilePath: root.loadedFilePath
        onPlayRequested: root.openRecording(filePath)
    }
}
