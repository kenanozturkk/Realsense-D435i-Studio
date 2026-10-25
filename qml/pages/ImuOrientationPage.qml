import QtQuick 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import RealSense.Ui 1.0
import "../components"
import "../theme"
import "imu"

Item {
    id: root

    onVisibleChanged: {
        if (visible) {
            anglesChart.clear()
            angularVelocityChart.clear()
        }
    }

    Connections {
        target: ImuController
        enabled: root.visible

        function onMeasurementsUpdated() {
            anglesChart.appendSample([ImuController.roll, ImuController.pitch, ImuController.yaw])
            const angularVelocity = ImuController.angularVelocity
            angularVelocityChart.appendSample([angularVelocity.x, angularVelocity.y, angularVelocity.z])
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXl
        spacing: Theme.spacingLg

        ImuPageHeader {
            Layout.fillWidth: true
        }

        Text {
            visible: ImuController.errorMessage.length > 0
            text: ImuController.errorMessage
            color: Theme.danger
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 3
            Layout.minimumHeight: readoutColumn.implicitHeight
            spacing: Theme.spacingLg

            Card {
                title: qsTr("3D Yönelim")
                subtitle: qsTr("Yeşil ok: kameranın baktığı yön · zemin halkası: yaw")
                Layout.fillWidth: true
                Layout.fillHeight: true

                OrientationView3D {
                    anchors.fill: parent
                    orientation: ImuController.orientation
                    bodyColor: "#7C8AA0"
                    frontColor: "#151C2C"
                    accentColor: Theme.accent
                    gridColor: Theme.border
                    labelColor: Theme.textSecondary
                }

                BiasCalibrationOverlay {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                }
            }

            ColumnLayout {
                id: readoutColumn
                Layout.fillWidth: false
                Layout.preferredWidth: 360
                Layout.fillHeight: true
                spacing: Theme.spacingMd

                AngleReadout {
                    Layout.fillWidth: true
                    label: qsTr("Roll")
                    description: qsTr("Sağa yatış +")
                    valueDegrees: ImuController.roll
                    accentColor: Theme.rollColor
                }

                AngleReadout {
                    Layout.fillWidth: true
                    label: qsTr("Pitch")
                    description: qsTr("Yukarı bakış +")
                    valueDegrees: ImuController.pitch
                    accentColor: Theme.pitchColor
                }

                AngleReadout {
                    Layout.fillWidth: true
                    label: qsTr("Yaw")
                    description: qsTr("Sola dönüş +")
                    valueDegrees: ImuController.yaw
                    accentColor: Theme.yawColor
                }

                SensorStatusCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: implicitHeight
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 2
            Layout.minimumHeight: 200
            spacing: Theme.spacingLg

            LiveChartCard {
                id: anglesChart
                title: qsTr("Açılar")
                subtitle: qsTr("son 10 saniye")
                unit: "°"
                minimumRange: 10
                series: [
                    { name: qsTr("Roll"), color: Theme.rollColor },
                    { name: qsTr("Pitch"), color: Theme.pitchColor },
                    { name: qsTr("Yaw"), color: Theme.yawColor }
                ]
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            LiveChartCard {
                id: angularVelocityChart
                title: qsTr("Açısal Hız")
                subtitle: qsTr("bias düzeltilmiş, °/s")
                unit: "°/s"
                minimumRange: 2
                series: [
                    { name: qsTr("İleri (X)"), color: Theme.axisXColor },
                    { name: qsTr("Sol (Y)"), color: Theme.axisYColor },
                    { name: qsTr("Yukarı (Z)"), color: Theme.axisZColor }
                ]
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
