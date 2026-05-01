import QtQuick

Item {
    id: root

    required property var theme
    required property var uiModel

    anchors.centerIn: parent
    visible: uiModel.crosshairVisible && uiModel.crosshairKeyToggleEnabled
        width: uiModel.crosshairLinesEnabled
           ? Math.max(96,
                    2 * (uiModel.crosshairHorizontalLength +
                        uiModel.crosshairGap +
                        uiModel.crosshairBorderThickness +
                        uiModel.crosshairThickness))
           : Math.max(24,
                    uiModel.crosshairCenterDotSize +
                    (uiModel.crosshairBorderEnabled ? 2 * uiModel.crosshairBorderThickness : 0) +
                    8)
        height: uiModel.crosshairLinesEnabled
           ? Math.max(72,
                    2 * (uiModel.crosshairVerticalLength +
                        uiModel.crosshairGap +
                        uiModel.crosshairBorderThickness +
                        uiModel.crosshairThickness))
           : Math.max(24,
                    uiModel.crosshairCenterDotSize +
                    (uiModel.crosshairBorderEnabled ? 2 * uiModel.crosshairBorderThickness : 0) +
                    8)

    property real centerX: width / 2
    property real centerY: height / 2
    property color crosshairColor: Qt.rgba(uiModel.crosshairColorRed,
                                           uiModel.crosshairColorGreen,
                                           uiModel.crosshairColorBlue,
                                           1.0)
    property color outlineColor: theme.window

    Rectangle {
        visible: uiModel.crosshairLinesEnabled && uiModel.crosshairBorderEnabled
        x: topCrosshairArm.x - uiModel.crosshairBorderThickness
        y: topCrosshairArm.y - uiModel.crosshairBorderThickness
        width: topCrosshairArm.width + 2 * uiModel.crosshairBorderThickness
        height: topCrosshairArm.height + 2 * uiModel.crosshairBorderThickness
        color: root.outlineColor
        z: 0
    }

    Rectangle {
        id: topCrosshairArm
        visible: uiModel.crosshairLinesEnabled
        x: root.centerX - uiModel.crosshairThickness / 2
        y: root.centerY - uiModel.crosshairGap - uiModel.crosshairVerticalLength
        width: uiModel.crosshairThickness
        height: uiModel.crosshairVerticalLength
        color: root.crosshairColor
        z: 1
    }

    Rectangle {
        visible: uiModel.crosshairLinesEnabled && uiModel.crosshairBorderEnabled
        x: bottomCrosshairArm.x - uiModel.crosshairBorderThickness
        y: bottomCrosshairArm.y - uiModel.crosshairBorderThickness
        width: bottomCrosshairArm.width + 2 * uiModel.crosshairBorderThickness
        height: bottomCrosshairArm.height + 2 * uiModel.crosshairBorderThickness
        color: root.outlineColor
        z: 0
    }

    Rectangle {
        id: bottomCrosshairArm
        visible: uiModel.crosshairLinesEnabled
        x: root.centerX - uiModel.crosshairThickness / 2
        y: root.centerY + uiModel.crosshairGap
        width: uiModel.crosshairThickness
        height: uiModel.crosshairVerticalLength
        color: root.crosshairColor
        z: 1
    }

    Rectangle {
        visible: uiModel.crosshairLinesEnabled && uiModel.crosshairBorderEnabled
        x: leftCrosshairArm.x - uiModel.crosshairBorderThickness
        y: leftCrosshairArm.y - uiModel.crosshairBorderThickness
        width: leftCrosshairArm.width + 2 * uiModel.crosshairBorderThickness
        height: leftCrosshairArm.height + 2 * uiModel.crosshairBorderThickness
        color: root.outlineColor
        z: 0
    }

    Rectangle {
        id: leftCrosshairArm
        visible: uiModel.crosshairLinesEnabled
        x: root.centerX - uiModel.crosshairGap - uiModel.crosshairHorizontalLength
        y: root.centerY - uiModel.crosshairThickness / 2
        width: uiModel.crosshairHorizontalLength
        height: uiModel.crosshairThickness
        color: root.crosshairColor
        z: 1
    }

    Rectangle {
        visible: uiModel.crosshairLinesEnabled && uiModel.crosshairBorderEnabled
        x: rightCrosshairArm.x - uiModel.crosshairBorderThickness
        y: rightCrosshairArm.y - uiModel.crosshairBorderThickness
        width: rightCrosshairArm.width + 2 * uiModel.crosshairBorderThickness
        height: rightCrosshairArm.height + 2 * uiModel.crosshairBorderThickness
        color: root.outlineColor
        z: 0
    }

    Rectangle {
        id: rightCrosshairArm
        visible: uiModel.crosshairLinesEnabled
        x: root.centerX + uiModel.crosshairGap
        y: root.centerY - uiModel.crosshairThickness / 2
        width: uiModel.crosshairHorizontalLength
        height: uiModel.crosshairThickness
        color: root.crosshairColor
        z: 1
    }

    Rectangle {
        visible: uiModel.crosshairCenterDotEnabled && uiModel.crosshairBorderEnabled
        x: crosshairCenterDot.x - uiModel.crosshairBorderThickness
        y: crosshairCenterDot.y - uiModel.crosshairBorderThickness
        width: crosshairCenterDot.width + 2 * uiModel.crosshairBorderThickness
        height: crosshairCenterDot.height + 2 * uiModel.crosshairBorderThickness
        radius: (crosshairCenterDot.width + 2 * uiModel.crosshairBorderThickness) / 2
        color: root.outlineColor
        z: 0
    }

    Rectangle {
        id: crosshairCenterDot
        visible: uiModel.crosshairCenterDotEnabled
        x: root.centerX - uiModel.crosshairCenterDotSize / 2
        y: root.centerY - uiModel.crosshairCenterDotSize / 2
        width: uiModel.crosshairCenterDotSize
        height: uiModel.crosshairCenterDotSize
        radius: width / 2
        color: root.crosshairColor
        z: 1
    }
}
