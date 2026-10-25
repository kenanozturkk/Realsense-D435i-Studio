import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import RealSense.Core 1.0
import "../../components"
import "../../theme"

Card {
    id: root

    title: qsTr("Ayarlar")

    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: settingsScroll.availableWidth
            spacing: Theme.spacingMd

            SectionHeader {
                text: qsTr("Akış")
            }

            SettingComboBox {
                Layout.fillWidth: true
                title: qsTr("Çözünürlük")
                model: DepthCameraController.resolutionPresetNames
                currentIndex: DepthCameraController.resolutionPresetIndex
                onActivated: DepthCameraController.resolutionPresetIndex = index
            }

            SettingSwitch {
                Layout.fillWidth: true
                title: qsTr("Derinliği renge hizala")
                description: qsTr("İki görüntüdeki aynı piksel aynı noktayı gösterir; renk görüntüsünde de ölçüm yapılabilir.")
                checked: DepthCameraController.isAlignedToColor
                onToggled: DepthCameraController.isAlignedToColor = isChecked
            }

            SectionHeader {
                text: qsTr("Görselleştirme")
            }

            SettingComboBox {
                Layout.fillWidth: true
                title: qsTr("Renk şeması")
                model: DepthCameraController.colorSchemeNames
                currentIndex: DepthCameraController.colorSchemeIndex
                onActivated: DepthCameraController.colorSchemeIndex = index
            }

            Text {
                text: qsTr("Renk aralığı: %1 – %2 m")
                          .arg(DepthCameraController.minimumDistance.toFixed(2))
                          .arg(DepthCameraController.maximumDistance.toFixed(2))
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }

            RangeSlider {
                Layout.fillWidth: true
                from: 0.1
                to: 10.0
                stepSize: 0.05
                first.value: DepthCameraController.minimumDistance
                second.value: DepthCameraController.maximumDistance
                first.onMoved: DepthCameraController.minimumDistance = first.value
                second.onMoved: DepthCameraController.maximumDistance = second.value
            }

            SectionHeader {
                text: qsTr("Filtreler")
            }

            SettingSwitch {
                Layout.fillWidth: true
                title: qsTr("Uzamsal filtre")
                description: qsTr("Kenarları koruyarak yüzeydeki gürültüyü yumuşatır.")
                checked: DepthCameraController.isSpatialFilterEnabled
                onToggled: DepthCameraController.isSpatialFilterEnabled = isChecked
            }

            SettingSwitch {
                Layout.fillWidth: true
                title: qsTr("Zamansal filtre")
                description: qsTr("Ardışık kareleri harmanlayarak titremeyi azaltır.")
                checked: DepthCameraController.isTemporalFilterEnabled
                onToggled: DepthCameraController.isTemporalFilterEnabled = isChecked
            }

            SettingSwitch {
                Layout.fillWidth: true
                title: qsTr("Delik doldurma")
                description: qsTr("Ölçülemeyen pikselleri komşularıyla doldurur; ölçüm doğruluğunu düşürebilir.")
                checked: DepthCameraController.isHoleFillingEnabled
                onToggled: DepthCameraController.isHoleFillingEnabled = isChecked
            }

            SectionHeader {
                text: qsTr("Projektör")
            }

            SettingComboBox {
                Layout.fillWidth: true
                title: qsTr("Intel görsel preset")
                model: DepthCameraController.visualPresetNames
                currentIndex: DepthCameraController.visualPresetIndex
                onActivated: DepthCameraController.visualPresetIndex = index
            }

            SettingSwitch {
                Layout.fillWidth: true
                title: qsTr("Kızılötesi projektör")
                description: qsTr("Desen yansıtarak düz ve dokusuz yüzeylerde derinliği iyileştirir.")
                checked: DepthCameraController.isEmitterEnabled
                onToggled: DepthCameraController.isEmitterEnabled = isChecked
            }

            Text {
                text: qsTr("Lazer gücü: %1 mW").arg(Math.round(DepthCameraController.laserPower))
                color: DepthCameraController.isEmitterEnabled ? Theme.textSecondary : Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeCaption
            }

            Slider {
                Layout.fillWidth: true
                enabled: DepthCameraController.isEmitterEnabled
                from: 0
                to: DepthCameraController.laserPowerMaximum
                stepSize: 30
                value: DepthCameraController.laserPower
                onMoved: DepthCameraController.laserPower = value
            }
        }
    }
}
