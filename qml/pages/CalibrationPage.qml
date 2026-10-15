import QtQuick 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import "../components"
import "../theme"
import "calibration"

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
                    text: qsTr("İvmeölçer Kalibrasyonu")
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeTitle
                    font.weight: Font.DemiBold
                }

                Text {
                    text: qsTr("6 yönlü kalibrasyon · eksen başına ofset ve ölçek düzeltmesi")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBody
                }
            }

            StatusBadge {
                text: CalibrationController.hasStoredCalibration ? qsTr("Kalibrasyon uygulanıyor") : qsTr("Kalibrasyon yok")
                statusColor: CalibrationController.hasStoredCalibration ? Theme.accent : Theme.warning
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: {
                switch (CalibrationController.wizardState) {
                case CalibrationController.Capturing: return 1
                case CalibrationController.ResultReady: return 2
                default: return 0
                }
            }

            CalibrationOverviewView {}
            CalibrationCaptureView {}
            CalibrationResultView {}
        }
    }
}
