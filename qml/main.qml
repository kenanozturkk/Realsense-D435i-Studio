import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import "pages"
import "theme"

ApplicationWindow {
    id: window

    width: 1440
    height: 900
    minimumWidth: 1180
    minimumHeight: 760
    visible: true
    title: qsTr("RealSense Studio")
    color: Theme.background
    font.family: Theme.fontFamily

    ListModel {
        id: navigationModel

        ListElement { title: qsTr("IMU"); iconPath: "qrc:/resources/icons/compass.svg" }
        ListElement { title: qsTr("Kalibrasyon"); iconPath: "qrc:/resources/icons/sliders-horizontal.svg" }
        ListElement { title: qsTr("Derinlik"); iconPath: "qrc:/resources/icons/box.svg" }
        ListElement { title: qsTr("Kayıt"); iconPath: "qrc:/resources/icons/circle-dot.svg" }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        SideNavigation {
            id: sideNavigation
            model: navigationModel
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            DeviceStatusBar {
                Layout.fillWidth: true
            }

            StackLayout {
                currentIndex: sideNavigation.currentIndex
                Layout.fillWidth: true
                Layout.fillHeight: true

                ImuOrientationPage {}

                CalibrationPage {}

                DepthPage {}

                RecordingPage {}
            }
        }
    }
}
