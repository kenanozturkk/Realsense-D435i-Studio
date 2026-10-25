import QtQuick 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import "../../components"
import "../../theme"

Card {
    id: root

    function formatVector(vector, decimals) {
        return [vector.x, vector.y, vector.z].map(function (component) {
            return (component >= 0 ? "+" : "") + component.toFixed(decimals)
        }).join("  ")
    }

    readonly property bool hasLiveData: ImuController.trackingState === ImuController.Calibrating
                                        || ImuController.trackingState === ImuController.Tracking

    title: qsTr("Sensör Durumu")
    contentImplicitHeight: statusLayout.implicitHeight

    ColumnLayout {
        id: statusLayout
        anchors.fill: parent
        spacing: Theme.spacingSm

        MetricRow {
            Layout.fillWidth: true
            label: qsTr("Hareket")
            value: !root.hasLiveData ? "—" : (ImuController.isStationary ? qsTr("Durağan") : qsTr("Hareketli"))
            valueColor: !root.hasLiveData ? Theme.textMuted : (ImuController.isStationary ? Theme.accent : Theme.warning)
        }

        MetricRow {
            Layout.fillWidth: true
            label: qsTr("Gyro / Accel hızı")
            value: qsTr("%1 / %2 Hz").arg(ImuController.gyroRate).arg(ImuController.accelRate)
        }

        MetricRow {
            Layout.fillWidth: true
            label: qsTr("İvmeölçer kalibrasyonu")
            value: CalibrationController.hasStoredCalibration ? qsTr("Uygulanıyor") : qsTr("Yok")
            valueColor: CalibrationController.hasStoredCalibration ? Theme.accent : Theme.warning
        }

        MetricRow {
            Layout.fillWidth: true
            label: qsTr("|g| ölçülen")
            value: root.hasLiveData ? qsTr("%1 m/s²").arg(ImuController.gravityMagnitude.toFixed(3)) : "—"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderSubtle
        }

        Text {
            text: qsTr("Gyro bias (°/s) · ileri  sol  yukarı")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
        }

        Text {
            text: root.hasLiveData ? root.formatVector(ImuController.gyroBias, 3) : "—"
            color: Theme.textPrimary
            font.family: Theme.monoFontFamily
            font.pixelSize: Theme.fontSizeBody
        }

        Text {
            text: qsTr("İvme (m/s²)")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
        }

        Text {
            text: root.hasLiveData ? root.formatVector(ImuController.acceleration, 2) : "—"
            color: Theme.textPrimary
            font.family: Theme.monoFontFamily
            font.pixelSize: Theme.fontSizeBody
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
