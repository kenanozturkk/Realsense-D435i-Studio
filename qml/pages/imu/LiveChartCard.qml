import QtQuick 2.15
import QtQuick.Layouts 1.15
import RealSense.Ui 1.0
import "../../components"
import "../../theme"

Card {
    id: root

    property var series: []
    property string unit
    property real minimumRange: 1

    function appendSample(values) {
        chart.appendSample(values)
    }

    function clear() {
        chart.clear()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingSm

        ChartLegend {
            entries: root.series
        }

        RealtimeChart {
            id: chart
            Layout.fillWidth: true
            Layout.fillHeight: true
            seriesColors: root.series.map(function (entry) { return entry.color })
            gridColor: Theme.borderSubtle
            labelColor: Theme.textMuted
            unit: root.unit
            minimumRange: root.minimumRange
            timeWindowSeconds: 10
        }
    }
}
