import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import "../../components"
import "../../theme"

Card {
    id: root

    property string selectedFilePath

    signal playRequested(string filePath)

    title: qsTr("Kayıtlar")
    subtitle: qsTr("%1 dosya").arg(RecordingController.recordings.length)

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingSm

        ListView {
            id: recordingsList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spacingXs
            model: RecordingController.recordings
            ScrollBar.vertical: ScrollBar {}

            delegate: ItemDelegate {
                required property var modelData

                width: recordingsList.width
                highlighted: modelData.filePath === root.selectedFilePath
                onDoubleClicked: root.playRequested(modelData.filePath)

                contentItem: RowLayout {
                    spacing: Theme.spacingSm

                    Column {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            width: parent.width
                            text: modelData.fileName
                            color: Theme.textPrimary
                            elide: Text.ElideMiddle
                            font.family: Theme.monoFontFamily
                            font.pixelSize: Theme.fontSizeCaption
                        }

                        Text {
                            text: qsTr("%1 MB · %2").arg(modelData.sizeMegabytes.toFixed(1)).arg(modelData.modifiedText)
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeCaption
                        }
                    }

                    ToolButton {
                        icon.source: "qrc:/resources/icons/play.svg"
                        icon.width: 16
                        icon.height: 16
                        Accessible.name: qsTr("Oynat")
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Oynat")
                        onClicked: root.playRequested(modelData.filePath)
                    }

                    ToolButton {
                        icon.source: "qrc:/resources/icons/trash-2.svg"
                        icon.width: 16
                        icon.height: 16
                        icon.color: Theme.danger
                        enabled: !RecordingController.isRecording
                        Accessible.name: qsTr("Sil")
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Sil")
                        onClicked: {
                            deleteDialog.filePath = modelData.filePath
                            deleteDialog.fileName = modelData.fileName
                            deleteDialog.open()
                        }
                    }
                }
            }
        }

        Text {
            visible: RecordingController.recordings.length === 0
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Henüz kayıt yok.")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
        }

        ActionButton {
            text: qsTr("Klasörü Aç")
            iconSource: "qrc:/resources/icons/folder-open.svg"
            toolTipText: RecordingController.recordingsDirectory
            onClicked: RecordingController.openRecordingsDirectory()
        }
    }

    Dialog {
        id: deleteDialog

        property string filePath
        property string fileName

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 440
        modal: true
        title: qsTr("Kayıt silinsin mi?")
        standardButtons: Dialog.Cancel | Dialog.Yes

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("%1 kalıcı olarak silinecek.").arg(deleteDialog.fileName)
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
        }

        onAccepted: {
            if (PlaybackController.isLoaded && root.selectedFilePath === filePath)
                PlaybackController.close()
            RecordingController.deleteRecording(filePath)
        }
    }
}
