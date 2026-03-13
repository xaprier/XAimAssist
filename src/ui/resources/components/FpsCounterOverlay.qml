import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    required property var theme
    required property var uiModel

    function fmtInt(value) {
        return Number(value || 0).toFixed(0)
    }

    function fmtMs(value) {
        return Number(value || 0).toFixed(2)
    }

    visible: uiModel.fpsCounterEnabled
    width: 186
    height: 56
    radius: 10
    color: theme.surface
    border.color: theme.border

    anchors.margins: 16
    anchors.top: uiModel.fpsCounterPosition === "top_left" || uiModel.fpsCounterPosition === "top_right"
        ? parent.top
        : undefined
    anchors.bottom: uiModel.fpsCounterPosition === "bottom_left" || uiModel.fpsCounterPosition === "bottom_right"
        ? parent.bottom
        : undefined
    anchors.left: uiModel.fpsCounterPosition === "top_left" || uiModel.fpsCounterPosition === "bottom_left"
        ? parent.left
        : undefined
    anchors.right: uiModel.fpsCounterPosition === "top_right" || uiModel.fpsCounterPosition === "bottom_right"
        ? parent.right
        : undefined

    Column {
        anchors.centerIn: parent
        spacing: 2

        Label {
            text: "FPS " + root.fmtInt(uiModel.currentFps)
            color: theme.textPrimary
            font.bold: true
        }

        Label {
            text: "FT " + root.fmtMs(uiModel.performanceStats.averageFrameTimeMs)
                + " ms  P95 " + root.fmtMs(uiModel.performanceStats.p95FrameTimeMs)
            color: theme.textSecondary
            font.pixelSize: 11
        }
    }
}
