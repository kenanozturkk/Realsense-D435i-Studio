import QtQuick 2.15
import QtGraphicalEffects 1.15

Item {
    id: root

    property url source
    property color color: "white"
    property int size: 20

    implicitWidth: size
    implicitHeight: size
    Accessible.ignored: true

    Image {
        id: iconImage
        anchors.fill: parent
        source: root.source
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        smooth: true
        visible: false
    }

    ColorOverlay {
        anchors.fill: iconImage
        source: iconImage
        color: root.color
    }
}
