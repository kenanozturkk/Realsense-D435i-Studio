import QtQuick 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import "../theme"

Rectangle {
    id: root

    implicitHeight: 56
    color: Theme.background

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.borderSubtle
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingXl
        anchors.rightMargin: Theme.spacingXl
        spacing: Theme.spacingMd

        Text {
            text: qsTr("RealSense Studio")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSubtitle
            font.weight: Font.DemiBold
        }

        Item {
            Layout.preferredWidth: Theme.spacingSm
        }

        StatusBadge {
            text: DeviceManager.isConnected ? DeviceManager.deviceName : qsTr("Cihaz bağlı değil")
            statusColor: DeviceManager.isConnected ? Theme.accent : Theme.danger
            isPulsing: !DeviceManager.isConnected
        }

        InfoChip {
            visible: DeviceManager.isConnected
            label: qsTr("USB")
            value: DeviceManager.usbType
        }

        InfoChip {
            visible: DeviceManager.isConnected
            label: qsTr("FW")
            value: DeviceManager.firmwareVersion
        }

        InfoChip {
            visible: DeviceManager.isConnected
            label: qsTr("S/N")
            value: DeviceManager.serialNumber
        }

        Item {
            Layout.fillWidth: true
        }

        Text {
            visible: DeviceManager.isConnected && !DeviceManager.usbType.startsWith("3")
            text: qsTr("USB 2 bağlantısı: performans düşük olabilir")
            color: Theme.warning
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeCaption
        }
    }
}
