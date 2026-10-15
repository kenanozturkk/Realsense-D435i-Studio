import QtQuick 2.15
import RealSense.Ui 1.0
import "../../components"
import "../../theme"

Card {
    id: root

    property QtObject frameSource
    property int stream: FrameSource.Depth
    property string placeholderText

    VideoFrameView {
        id: frameView
        anchors.fill: parent
        source: root.frameSource
        stream: root.stream
    }

    Text {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.spacingLg
        horizontalAlignment: Text.AlignHCenter
        visible: !frameView.hasImage
        text: root.placeholderText
        color: Theme.textMuted
        wrapMode: Text.WordWrap
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeBody
    }
}
