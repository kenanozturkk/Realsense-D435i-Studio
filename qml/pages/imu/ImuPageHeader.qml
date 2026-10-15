import QtQuick 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import "../../components"
import "../../theme"

RowLayout {
    id: root

    readonly property bool isRunning: ImuController.trackingState === ImuController.Calibrating
                                      || ImuController.trackingState === ImuController.Tracking
    readonly property bool isActive: isRunning || ImuController.trackingState === ImuController.WaitingForDevice

    spacing: Theme.spacingMd

    Column {
        Layout.fillWidth: true
        spacing: 2

        Text {
            text: qsTr("IMU Yön Takibi")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeTitle
            font.weight: Font.DemiBold
        }

        Text {
            text: qsTr("Gyro + ivmeölçer füzyonu · Mahony filtresi · otomatik bias takibi")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
        }
    }

    StatusBadge {
        text: {
            switch (ImuController.trackingState) {
            case ImuController.WaitingForDevice: return qsTr("Cihaz bekleniyor")
            case ImuController.Calibrating: return qsTr("Kalibre ediliyor")
            case ImuController.Tracking: return qsTr("Takip ediliyor")
            case ImuController.Error: return qsTr("Hata")
            default: return qsTr("Durduruldu")
            }
        }
        statusColor: {
            switch (ImuController.trackingState) {
            case ImuController.Tracking: return Theme.accent
            case ImuController.Error: return Theme.danger
            case ImuController.Stopped: return Theme.textMuted
            default: return Theme.warning
            }
        }
        isPulsing: ImuController.trackingState === ImuController.Calibrating
                   || ImuController.trackingState === ImuController.WaitingForDevice
    }

    ActionButton {
        text: qsTr("Yönü Sıfırla")
        iconSource: "qrc:/resources/icons/rotate-ccw.svg"
        toolTipText: qsTr("Şu anki bakış yönünü yaw = 0° kabul eder")
        enabled: root.isRunning
        onClicked: ImuController.resetHeading()
    }

    ActionButton {
        text: qsTr("Bias'ı Yeniden Öğren")
        iconSource: "qrc:/resources/icons/crosshair.svg"
        toolTipText: qsTr("Kamerayı sabit bırakın; gyro sıfır noktası baştan hesaplanır")
        enabled: root.isRunning
        onClicked: ImuController.recalibrateGyroBias()
    }

    ActionButton {
        text: root.isActive ? qsTr("Durdur") : qsTr("Başlat")
        iconSource: root.isActive ? "qrc:/resources/icons/square.svg" : "qrc:/resources/icons/play.svg"
        isPrimary: !root.isActive
        enabled: !RecordingController.isRecording
        onClicked: root.isActive ? ImuController.stop() : ImuController.start()
    }
}
