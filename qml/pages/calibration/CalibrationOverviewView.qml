import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import "../../components"
import "../../theme"

RowLayout {
    id: root

    readonly property real liveGravity: ImuController.acceleration.length()
    readonly property real liveGravityError: liveGravity - 9.80665
    readonly property bool isImuRunning: ImuController.trackingState === ImuController.Calibrating
                                         || ImuController.trackingState === ImuController.Tracking

    spacing: Theme.spacingLg

    Card {
        title: qsTr("Neden kalibrasyon?")
        contentImplicitHeight: introLayout.implicitHeight
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop

        ColumnLayout {
            id: introLayout
            anchors.fill: parent
            spacing: Theme.spacingMd

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                lineHeight: 1.4
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                text: qsTr("D435i ivmeölçeri fabrikadan kalibre edilmeden gelir: duran kamerada yerçekimi 9.81 yerine "
                           + "yaklaşık 9.64 m/s² okunur. Bu sapma roll ve pitch açılarına küçük ama sürekli bir hata "
                           + "olarak yansır. Kalibrasyon her eksen için ofset ve ölçek düzeltmesi hesaplar.")
            }

            Text {
                text: qsTr("Nasıl yapılır?")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBody
                font.weight: Font.Medium
            }

            Repeater {
                model: [
                    qsTr("Kamerayı sırayla 6 farklı yüzü üstte kalacak şekilde sabit bir yüzeye koyun."),
                    qsTr("Uygulama duruşu kendisi tanır; kamera 2 saniye kıpırdamadan durunca otomatik kaydeder."),
                    qsTr("Duruşları istediğiniz sırada yapabilirsiniz; tam dik olmaları gerekmez."),
                    qsTr("Kabloyu gergin bırakmayın, kamerayı çekmesin.")
                ]

                delegate: Text {
                    required property string modelData
                    required property int index

                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBody
                    text: qsTr("%1.  %2").arg(index + 1).arg(modelData)
                }
            }

            Item {
                Layout.preferredHeight: Theme.spacingSm
            }

            ActionButton {
                text: qsTr("Kalibrasyonu Başlat")
                iconSource: "qrc:/resources/icons/play.svg"
                isPrimary: true
                enabled: DeviceManager.isConnected
                onClicked: CalibrationController.startWizard()
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: false
        Layout.preferredWidth: 420
        Layout.fillHeight: true
        spacing: Theme.spacingLg

        Card {
            title: qsTr("Kayıtlı kalibrasyon")
            subtitle: CalibrationController.hasStoredCalibration ? CalibrationController.storedCalibrationDate : ""
            contentImplicitHeight: storedLayout.implicitHeight
            Layout.fillWidth: true

            ColumnLayout {
                id: storedLayout
                anchors.fill: parent
                spacing: Theme.spacingMd

                Text {
                    visible: !CalibrationController.hasStoredCalibration
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: DeviceManager.isConnected ? qsTr("Bu kamera için kayıtlı kalibrasyon yok. Ham ivmeölçer değerleri kullanılıyor.")
                                                    : qsTr("Kamera bağlı değil.")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBody
                }

                CalibrationValuesTable {
                    visible: CalibrationController.hasStoredCalibration
                    Layout.fillWidth: true
                    offset: CalibrationController.storedOffset
                    scale: CalibrationController.storedScale
                }

                ActionButton {
                    visible: CalibrationController.hasStoredCalibration
                    text: qsTr("Kalibrasyonu Sil")
                    iconSource: "qrc:/resources/icons/trash-2.svg"
                    toolTipText: qsTr("Kayıtlı düzeltmeyi siler ve ham değerlere döner")
                    onClicked: clearConfirmationDialog.open()
                }
            }
        }

        Card {
            title: qsTr("Canlı doğrulama")
            subtitle: qsTr("her açıda 9.81'e yakın olmalı")
            contentImplicitHeight: verificationLayout.implicitHeight
            Layout.fillWidth: true

            ColumnLayout {
                id: verificationLayout
                anchors.fill: parent
                spacing: Theme.spacingSm

                Text {
                    text: root.isImuRunning ? qsTr("%1 m/s²").arg(root.liveGravity.toFixed(3)) : qsTr("IMU çalışmıyor")
                    color: Math.abs(root.liveGravityError) < 0.03 ? Theme.accent : Theme.warning
                    font.family: Theme.monoFontFamily
                    font.pixelSize: Theme.fontSizeReadout
                    font.weight: Font.Medium
                }

                Text {
                    visible: root.isImuRunning
                    text: qsTr("Sapma: %1%2 m/s²  ·  %3%")
                              .arg(root.liveGravityError >= 0 ? "+" : "")
                              .arg(root.liveGravityError.toFixed(3))
                              .arg((Math.abs(root.liveGravityError) / 9.80665 * 100).toFixed(2))
                    color: Theme.textSecondary
                    font.family: Theme.monoFontFamily
                    font.pixelSize: Theme.fontSizeCaption
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }

    Dialog {
        id: clearConfirmationDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: 440
        modal: true
        title: qsTr("Kalibrasyon silinsin mi?")
        standardButtons: Dialog.Cancel | Dialog.Yes
        onAccepted: CalibrationController.clearStoredCalibration()

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Bu kameranın kayıtlı ivmeölçer düzeltmesi silinecek ve ham değerlere dönülecek.\nGeri almak için kalibrasyonu yeniden yapmanız gerekir.")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBody
        }
    }
}
