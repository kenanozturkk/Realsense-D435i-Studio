import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import "../components"
import "../theme"
import "media"

Item {
    id: root

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
                    text: qsTr("Kayıt ve Oynatma")
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeTitle
                    font.weight: Font.DemiBold
                }

                Text {
                    text: qsTr("Akışları .bag dosyasına kaydedin, kamera takılı olmadan tekrar oynatın")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBody
                }
            }

            TabBar {
                id: modeTabs
                Layout.preferredWidth: 280
                background: Item {}

                TabButton {
                    text: qsTr("Kayıt")
                }

                TabButton {
                    text: qsTr("Oynatma")
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: modeTabs.currentIndex

            RecordTab {
                onPlayRequested: {
                    modeTabs.currentIndex = 1
                    playbackTab.openRecording(filePath)
                }
            }

            PlaybackTab {
                id: playbackTab
            }
        }
    }
}
