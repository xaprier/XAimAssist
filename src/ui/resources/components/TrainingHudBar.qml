import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var theme
    required property var i18n
    required property var uiModel

    function fmtInt(value) {
        return Number(value || 0).toFixed(0)
    }

    function fmtSeconds(value) {
        return Number(value || 0).toFixed(1) + " " + i18n.unitSec
    }

    anchors.top: parent.top
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.topMargin: 16
    width: 520
    height: 60
    radius: 12
    color: theme.surface
    border.color: theme.border

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10

        Label { text: i18n.trainingScore + ": " + root.fmtInt(uiModel.realtimeStats.score); color: theme.textPrimary }
        Item { Layout.fillWidth: true }
        Label { text: i18n.trainingHits + ": " + root.fmtInt(uiModel.realtimeStats.hits); color: theme.textPrimary }
        Item { Layout.fillWidth: true }
        Label { text: i18n.trainingMisses + ": " + root.fmtInt(uiModel.realtimeStats.misses); color: theme.textPrimary }
        Item { Layout.fillWidth: true }
        Label { text: i18n.trainingElapsed + ": " + root.fmtSeconds(uiModel.realtimeStats.elapsedSeconds); color: theme.textPrimary }
    }
}
