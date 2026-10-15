import QtQuick 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import "../../components"
import "../../theme"

Item {
    id: root

    Card {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        width: Math.min(parent.width, 620)
        title: CalibrationController.isResultValid ? qsTr("Kalibrasyon hesaplandı") : qsTr("Kalibrasyon başarısız")
        contentImplicitHeight: resultLayout.implicitHeight

        ColumnLayout {
            id: resultLayout
            anchors.fill: parent
            spacing: Theme.spacingLg

            Text {
                visible: !CalibrationController.isResultValid
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: CalibrationController.resultFailureReason
                color: Theme.danger
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
            }

            CalibrationValuesTable {
                visible: CalibrationController.isResultValid
                Layout.fillWidth: true
                offset: CalibrationController.resultOffset
                scale: CalibrationController.resultScale
            }

            MetricRow {
                Layout.fillWidth: true
                label: qsTr("Kalibrasyon öncesi |g| hatası (RMS)")
                value: qsTr("%1 m/s²").arg(CalibrationController.resultRawError.toFixed(3))
                valueColor: Theme.warning
            }

            Text {
                visible: CalibrationController.isResultValid
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Kaydettikten sonra düzeltme hemen uygulanır ve bu kamera her takıldığında otomatik yüklenir. "
                           + "Genel bakış ekranındaki canlı doğrulama ile sonucu kontrol edebilirsiniz.")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                lineHeight: 1.4
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMd

                Item {
                    Layout.fillWidth: true
                }

                ActionButton {
                    text: qsTr("Yeniden Yap")
                    iconSource: "qrc:/resources/icons/refresh-cw.svg"
                    onClicked: CalibrationController.startWizard()
                }

                ActionButton {
                    text: qsTr("Vazgeç")
                    iconSource: "qrc:/resources/icons/x.svg"
                    onClicked: CalibrationController.discardResult()
                }

                ActionButton {
                    visible: CalibrationController.isResultValid
                    text: qsTr("Kaydet ve Uygula")
                    iconSource: "qrc:/resources/icons/save.svg"
                    isPrimary: true
                    onClicked: CalibrationController.saveResult()
                }
            }
        }
    }
}
