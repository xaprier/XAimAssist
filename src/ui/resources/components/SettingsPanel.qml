import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Item {
    id: root

    required property var theme
    required property var i18n
    required property var viewModel

    readonly property bool isLightTheme: !!(root.viewModel && root.viewModel.themeMode === "light")

    Material.theme: root.isLightTheme ? Material.Light : Material.Dark
    Material.accent: root.theme.accent
    Material.primary: root.theme.accent
    Material.background: root.theme.surface
    Material.foreground: root.theme.textPrimary

    function fmtInt(value) {
        return Number(value || 0).toFixed(0)
    }

    function fpsPositionModel() {
        return [
            { label: i18n.settingsFpsTopRight, value: "top_right" },
            { label: i18n.settingsFpsTopLeft, value: "top_left" },
            { label: i18n.settingsFpsBottomRight, value: "bottom_right" },
            { label: i18n.settingsFpsBottomLeft, value: "bottom_left" }
        ]
    }

    function fpsPositionIndex(value) {
        const positions = fpsPositionModel()
        for (let i = 0; i < positions.length; ++i) {
            if (positions[i].value === value) {
                return i
            }
        }

        return 0
    }

    function fpsPositionValue(index) {
        const positions = fpsPositionModel()
        if (index < 0 || index >= positions.length) {
            return "top_right"
        }

        return positions[index].value
    }

    readonly property bool wideLayout: settingsColumn.width >= 720

    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        contentHeight: settingsColumn.implicitHeight

        ColumnLayout {
            id: settingsColumn
            width: settingsScroll.availableWidth
            spacing: 10

            Rectangle {
                Layout.fillWidth: true
                radius: 14
                color: root.theme.surface
                border.color: root.theme.border
                implicitHeight: themeSettingsColumn.implicitHeight + 28

                ColumnLayout {
                    id: themeSettingsColumn
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.wideLayout ? 2 : 1
                        columnSpacing: 12
                        rowSpacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true

                            Label {
                                text: root.i18n.settingsTheme
                                color: root.theme.textSecondary
                            }

                            ComboBox {
                                Layout.fillWidth: true
                                model: [
                                    { label: root.i18n.settingsThemeDark, value: "dark" },
                                    { label: root.i18n.settingsThemeLight, value: "light" }
                                ]
                                textRole: "label"
                                currentIndex: root.viewModel.themeMode === "light" ? 1 : 0
                                onActivated: function(comboIndex) {
                                    root.viewModel.themeMode = model[comboIndex].value
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true

                            Label {
                                text: root.i18n.settingsLanguage
                                color: root.theme.textSecondary
                            }

                            ComboBox {
                                Layout.fillWidth: true
                                model: [
                                    { label: root.i18n.settingsLanguageEn, value: "en" },
                                    { label: root.i18n.settingsLanguageTr, value: "tr" }
                                ]
                                textRole: "label"
                                currentIndex: root.viewModel.languageCode === "tr" ? 1 : 0
                                onModelChanged: currentIndex = root.viewModel.languageCode === "tr" ? 1 : 0
                                onActivated: function(comboIndex) {
                                    root.viewModel.languageCode = model[comboIndex].value
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 14
                color: root.theme.surface
                border.color: root.theme.border
                implicitHeight: inputSettingsColumn.implicitHeight + 28

                ColumnLayout {
                    id: inputSettingsColumn
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Label { text: root.i18n.settingsInput; color: root.theme.textPrimary; font.bold: true }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.wideLayout ? 2 : 1
                        columnSpacing: 14
                        rowSpacing: 8

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: root.i18n.settingsInvertY
                                color: root.theme.textSecondary
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                wrapMode: Text.WordWrap
                            }

                            Switch {
                                checked: root.viewModel.invertY
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                onToggled: root.viewModel.invertY = checked
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: root.i18n.settingsRawInput
                                color: root.theme.textSecondary
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                wrapMode: Text.WordWrap
                            }

                            Switch {
                                checked: root.viewModel.rawInputEnabled
                                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                onToggled: root.viewModel.rawInputEnabled = checked
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true

                            Label { text: root.i18n.settingsSensitivityScale + ": " + Number(root.viewModel.sensitivityScale).toFixed(2); color: root.theme.textSecondary }
                            Slider {
                                Layout.fillWidth: true
                                from: 0.1
                                to: 3.0
                                stepSize: 0.01
                                value: root.viewModel.sensitivityScale
                                onValueChanged: {
                                    if (pressed && Math.abs(root.viewModel.sensitivityScale - value) > 0.0001) {
                                        root.viewModel.sensitivityScale = value
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true

                            Label { text: root.i18n.settingsCm360 + ": " + Number(root.viewModel.cmPer360).toFixed(2); color: root.theme.textSecondary }
                            Slider {
                                Layout.fillWidth: true
                                from: 10
                                to: 80
                                stepSize: 0.1
                                value: root.viewModel.cmPer360
                                onValueChanged: {
                                    if (pressed && Math.abs(root.viewModel.cmPer360 - value) > 0.0001) {
                                        root.viewModel.cmPer360 = value
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true

                            Label { text: root.i18n.settingsDpi + ": " + root.fmtInt(root.viewModel.dpi); color: root.theme.textSecondary }
                            Slider {
                                Layout.fillWidth: true
                                from: 200
                                to: 3200
                                stepSize: 10
                                value: root.viewModel.dpi
                                onValueChanged: {
                                    if (pressed && Math.abs(root.viewModel.dpi - value) > 0.0001) {
                                        root.viewModel.dpi = value
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 14
                color: root.theme.surface
                border.color: root.theme.border
                implicitHeight: crosshairSettingsColumn.implicitHeight + 28

                ColumnLayout {
                    id: crosshairSettingsColumn
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Label {
                        text: root.i18n.settingsCrosshair
                        color: root.theme.textPrimary
                        font.bold: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 94
                        radius: 10
                        color: root.theme.window
                        border.color: root.theme.border

                        Item {
                            id: previewCrosshair
                            anchors.centerIn: parent
                               width: root.viewModel.crosshairLinesEnabled
                                  ? Math.max(96,
                                           2 * (root.viewModel.crosshairHorizontalLength +
                                               root.viewModel.crosshairGap +
                                               root.viewModel.crosshairBorderThickness +
                                               root.viewModel.crosshairThickness))
                                  : Math.max(24,
                                           root.viewModel.crosshairCenterDotSize +
                                           (root.viewModel.crosshairBorderEnabled ? 2 * root.viewModel.crosshairBorderThickness : 0) +
                                           8)
                               height: root.viewModel.crosshairLinesEnabled
                                  ? Math.max(72,
                                           2 * (root.viewModel.crosshairVerticalLength +
                                               root.viewModel.crosshairGap +
                                               root.viewModel.crosshairBorderThickness +
                                               root.viewModel.crosshairThickness))
                                  : Math.max(24,
                                           root.viewModel.crosshairCenterDotSize +
                                           (root.viewModel.crosshairBorderEnabled ? 2 * root.viewModel.crosshairBorderThickness : 0) +
                                           8)

                            property real centerX: width / 2
                            property real centerY: height / 2
                            property color crosshairColor: Qt.rgba(root.viewModel.crosshairColorRed,
                                                                   root.viewModel.crosshairColorGreen,
                                                                   root.viewModel.crosshairColorBlue,
                                                                   1.0)
                            property color outlineColor: root.theme.surface

                            Rectangle {
                                visible: root.viewModel.crosshairLinesEnabled && root.viewModel.crosshairBorderEnabled
                                x: topArm.x - root.viewModel.crosshairBorderThickness
                                y: topArm.y - root.viewModel.crosshairBorderThickness
                                width: topArm.width + 2 * root.viewModel.crosshairBorderThickness
                                height: topArm.height + 2 * root.viewModel.crosshairBorderThickness
                                color: previewCrosshair.outlineColor
                                z: 0
                            }
                            Rectangle {
                                id: topArm
                                visible: root.viewModel.crosshairLinesEnabled
                                x: previewCrosshair.centerX - root.viewModel.crosshairThickness / 2
                                y: previewCrosshair.centerY - root.viewModel.crosshairGap - root.viewModel.crosshairVerticalLength
                                width: root.viewModel.crosshairThickness
                                height: root.viewModel.crosshairVerticalLength
                                color: previewCrosshair.crosshairColor
                                z: 1
                            }

                            Rectangle {
                                visible: root.viewModel.crosshairLinesEnabled && root.viewModel.crosshairBorderEnabled
                                x: bottomArm.x - root.viewModel.crosshairBorderThickness
                                y: bottomArm.y - root.viewModel.crosshairBorderThickness
                                width: bottomArm.width + 2 * root.viewModel.crosshairBorderThickness
                                height: bottomArm.height + 2 * root.viewModel.crosshairBorderThickness
                                color: previewCrosshair.outlineColor
                                z: 0
                            }
                            Rectangle {
                                id: bottomArm
                                visible: root.viewModel.crosshairLinesEnabled
                                x: previewCrosshair.centerX - root.viewModel.crosshairThickness / 2
                                y: previewCrosshair.centerY + root.viewModel.crosshairGap
                                width: root.viewModel.crosshairThickness
                                height: root.viewModel.crosshairVerticalLength
                                color: previewCrosshair.crosshairColor
                                z: 1
                            }

                            Rectangle {
                                visible: root.viewModel.crosshairLinesEnabled && root.viewModel.crosshairBorderEnabled
                                x: leftArm.x - root.viewModel.crosshairBorderThickness
                                y: leftArm.y - root.viewModel.crosshairBorderThickness
                                width: leftArm.width + 2 * root.viewModel.crosshairBorderThickness
                                height: leftArm.height + 2 * root.viewModel.crosshairBorderThickness
                                color: previewCrosshair.outlineColor
                                z: 0
                            }
                            Rectangle {
                                id: leftArm
                                visible: root.viewModel.crosshairLinesEnabled
                                x: previewCrosshair.centerX - root.viewModel.crosshairGap - root.viewModel.crosshairHorizontalLength
                                y: previewCrosshair.centerY - root.viewModel.crosshairThickness / 2
                                width: root.viewModel.crosshairHorizontalLength
                                height: root.viewModel.crosshairThickness
                                color: previewCrosshair.crosshairColor
                                z: 1
                            }

                            Rectangle {
                                visible: root.viewModel.crosshairLinesEnabled && root.viewModel.crosshairBorderEnabled
                                x: rightArm.x - root.viewModel.crosshairBorderThickness
                                y: rightArm.y - root.viewModel.crosshairBorderThickness
                                width: rightArm.width + 2 * root.viewModel.crosshairBorderThickness
                                height: rightArm.height + 2 * root.viewModel.crosshairBorderThickness
                                color: previewCrosshair.outlineColor
                                z: 0
                            }
                            Rectangle {
                                id: rightArm
                                visible: root.viewModel.crosshairLinesEnabled
                                x: previewCrosshair.centerX + root.viewModel.crosshairGap
                                y: previewCrosshair.centerY - root.viewModel.crosshairThickness / 2
                                width: root.viewModel.crosshairHorizontalLength
                                height: root.viewModel.crosshairThickness
                                color: previewCrosshair.crosshairColor
                                z: 1
                            }

                            Rectangle {
                                visible: root.viewModel.crosshairCenterDotEnabled && root.viewModel.crosshairBorderEnabled
                                x: centerDot.x - root.viewModel.crosshairBorderThickness
                                y: centerDot.y - root.viewModel.crosshairBorderThickness
                                width: centerDot.width + 2 * root.viewModel.crosshairBorderThickness
                                height: centerDot.height + 2 * root.viewModel.crosshairBorderThickness
                                radius: (centerDot.width + 2 * root.viewModel.crosshairBorderThickness) / 2
                                color: previewCrosshair.outlineColor
                                z: 0
                            }
                            Rectangle {
                                id: centerDot
                                visible: root.viewModel.crosshairCenterDotEnabled
                                x: previewCrosshair.centerX - root.viewModel.crosshairCenterDotSize / 2
                                y: previewCrosshair.centerY - root.viewModel.crosshairCenterDotSize / 2
                                width: root.viewModel.crosshairCenterDotSize
                                height: root.viewModel.crosshairCenterDotSize
                                radius: width / 2
                                color: previewCrosshair.crosshairColor
                                z: 1
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: root.theme.window
                        border.color: root.theme.border
                        implicitHeight: centerDotGroup.implicitHeight + 20

                        ColumnLayout {
                            id: centerDotGroup
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Label {
                                text: root.i18n.settingsCrosshairCenterDot
                                color: root.theme.textPrimary
                                font.bold: true
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: root.wideLayout ? 4 : 2
                                columnSpacing: 12
                                rowSpacing: 8

                                Label {
                                    text: root.i18n.settingsEnable
                                    color: root.theme.textSecondary
                                    Layout.alignment: Qt.AlignVCenter
                                }

                                Switch {
                                    checked: root.viewModel.crosshairCenterDotEnabled
                                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                    onToggled: root.viewModel.crosshairCenterDotEnabled = checked
                                }

                                Label {
                                    text: root.i18n.settingsSize + ": " + Number(root.viewModel.crosshairCenterDotSize).toFixed(1)
                                    color: root.theme.textSecondary
                                    Layout.alignment: Qt.AlignVCenter
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    from: 1.0
                                    to: 16.0
                                    stepSize: 0.1
                                    value: root.viewModel.crosshairCenterDotSize
                                    enabled: root.viewModel.crosshairCenterDotEnabled
                                    onValueChanged: {
                                        if (pressed && Math.abs(root.viewModel.crosshairCenterDotSize - value) > 0.0001) {
                                            root.viewModel.crosshairCenterDotSize = value
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: root.theme.window
                        border.color: root.theme.border
                        implicitHeight: outlineGroup.implicitHeight + 20

                        ColumnLayout {
                            id: outlineGroup
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Label {
                                text: root.i18n.settingsCrosshairBorder
                                color: root.theme.textPrimary
                                font.bold: true
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: root.wideLayout ? 4 : 2
                                columnSpacing: 12
                                rowSpacing: 8

                                Label {
                                    text: root.i18n.settingsEnable
                                    color: root.theme.textSecondary
                                    Layout.alignment: Qt.AlignVCenter
                                }

                                Switch {
                                    checked: root.viewModel.crosshairBorderEnabled
                                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                    onToggled: root.viewModel.crosshairBorderEnabled = checked
                                }

                                Label {
                                    text: root.i18n.settingsSize + ": " + Number(root.viewModel.crosshairBorderThickness).toFixed(1)
                                    color: root.theme.textSecondary
                                    Layout.alignment: Qt.AlignVCenter
                                }

                                Slider {
                                    Layout.fillWidth: true
                                    from: 1.0
                                    to: 6.0
                                    stepSize: 0.1
                                    value: root.viewModel.crosshairBorderThickness
                                    enabled: root.viewModel.crosshairBorderEnabled
                                    onValueChanged: {
                                        if (pressed && Math.abs(root.viewModel.crosshairBorderThickness - value) > 0.0001) {
                                            root.viewModel.crosshairBorderThickness = value
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: 10
                        color: root.theme.window
                        border.color: root.theme.border
                        implicitHeight: linesGroup.implicitHeight + 20

                        ColumnLayout {
                            id: linesGroup
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Label {
                                text: root.i18n.settingsCrosshairLines
                                color: root.theme.textPrimary
                                font.bold: true
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    text: root.i18n.settingsEnable
                                    color: root.theme.textSecondary
                                    Layout.alignment: Qt.AlignVCenter
                                }

                                Switch {
                                    checked: root.viewModel.crosshairLinesEnabled
                                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                                    onToggled: root.viewModel.crosshairLinesEnabled = checked
                                }

                                Item {
                                    Layout.fillWidth: true
                                }
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: root.wideLayout ? 2 : 1
                                columnSpacing: 12
                                rowSpacing: 8

                                ColumnLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: root.i18n.settingsCrosshairThickness + ": " + Number(root.viewModel.crosshairThickness).toFixed(1)
                                        color: root.theme.textSecondary
                                    }
                                    Slider {
                                        Layout.fillWidth: true
                                        from: 1.0
                                        to: 12.0
                                        stepSize: 0.1
                                        value: root.viewModel.crosshairThickness
                                        enabled: root.viewModel.crosshairLinesEnabled
                                        onValueChanged: {
                                            if (pressed && Math.abs(root.viewModel.crosshairThickness - value) > 0.0001) {
                                                root.viewModel.crosshairThickness = value
                                            }
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: root.i18n.settingsCrosshairGap + ": " + Number(root.viewModel.crosshairGap).toFixed(1)
                                        color: root.theme.textSecondary
                                    }
                                    Slider {
                                        Layout.fillWidth: true
                                        from: 0.0
                                        to: 20.0
                                        stepSize: 0.1
                                        value: root.viewModel.crosshairGap
                                        enabled: root.viewModel.crosshairLinesEnabled
                                        onValueChanged: {
                                            if (pressed && Math.abs(root.viewModel.crosshairGap - value) > 0.0001) {
                                                root.viewModel.crosshairGap = value
                                            }
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: root.i18n.settingsCrosshairHorizontalLength + ": " + Number(root.viewModel.crosshairHorizontalLength).toFixed(1)
                                        color: root.theme.textSecondary
                                    }
                                    Slider {
                                        Layout.fillWidth: true
                                        from: 2.0
                                        to: 40.0
                                        stepSize: 0.1
                                        value: root.viewModel.crosshairHorizontalLength
                                        enabled: root.viewModel.crosshairLinesEnabled
                                        onValueChanged: {
                                            if (pressed && Math.abs(root.viewModel.crosshairHorizontalLength - value) > 0.0001) {
                                                root.viewModel.crosshairHorizontalLength = value
                                            }
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true

                                    Label {
                                        text: root.i18n.settingsCrosshairVerticalLength + ": " + Number(root.viewModel.crosshairVerticalLength).toFixed(1)
                                        color: root.theme.textSecondary
                                    }
                                    Slider {
                                        Layout.fillWidth: true
                                        from: 2.0
                                        to: 40.0
                                        stepSize: 0.1
                                        value: root.viewModel.crosshairVerticalLength
                                        enabled: root.viewModel.crosshairLinesEnabled
                                        onValueChanged: {
                                            if (pressed && Math.abs(root.viewModel.crosshairVerticalLength - value) > 0.0001) {
                                                root.viewModel.crosshairVerticalLength = value
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ColorEditor {
                        Layout.fillWidth: true
                        theme: root.theme
                        labelText: root.i18n.settingsCrosshairColor
                        red: root.viewModel.crosshairColorRed
                        green: root.viewModel.crosshairColorGreen
                        blue: root.viewModel.crosshairColorBlue
                        onColorEdited: function(red, green, blue) {
                            if (Math.abs(root.viewModel.crosshairColorRed - red) > 0.0001) {
                                root.viewModel.crosshairColorRed = red
                            }
                            if (Math.abs(root.viewModel.crosshairColorGreen - green) > 0.0001) {
                                root.viewModel.crosshairColorGreen = green
                            }
                            if (Math.abs(root.viewModel.crosshairColorBlue - blue) > 0.0001) {
                                root.viewModel.crosshairColorBlue = blue
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 14
                color: root.theme.surface
                border.color: root.theme.border
                implicitHeight: fpsSettingsColumn.implicitHeight + 28

                ColumnLayout {
                    id: fpsSettingsColumn
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Label {
                        text: root.i18n.settingsFps
                        color: root.theme.textPrimary
                        font.bold: true
                    }

                    RowLayout {
                        ColumnLayout {
                            Layout.fillWidth: true

                            Label {
                                text: root.i18n.settingsFpsEnabled
                                color: root.theme.textSecondary
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            Switch {
                                checked: root.viewModel.fpsCounterEnabled
                                onToggled: root.viewModel.fpsCounterEnabled = checked
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true

                            Label {
                                text: root.i18n.settingsFpsPosition
                                color: root.theme.textSecondary
                            }

                            ComboBox {
                                    Layout.fillWidth: true
                                    model: root.fpsPositionModel()
                                    textRole: "label"
                                    enabled: root.viewModel.fpsCounterEnabled
                                    currentIndex: root.fpsPositionIndex(root.viewModel.fpsCounterPosition)
                                    onActivated: function(comboIndex) {
                                        root.viewModel.fpsCounterPosition = root.fpsPositionValue(comboIndex)
                                    }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 14
                color: root.theme.surface
                border.color: root.theme.border
                implicitHeight: targetSettingsColumn.implicitHeight + 28

                ColumnLayout {
                    id: targetSettingsColumn
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Label { text: root.i18n.settingsTargetColor; color: root.theme.textPrimary; font.bold: true }

                    ColorEditor {
                        Layout.fillWidth: true
                        theme: root.theme
                        labelText: root.i18n.settingsTargetColor
                        red: root.viewModel.targetColorRed
                        green: root.viewModel.targetColorGreen
                        blue: root.viewModel.targetColorBlue
                        onColorEdited: function(red, green, blue) {
                            if (Math.abs(root.viewModel.targetColorRed - red) > 0.0001) {
                                root.viewModel.targetColorRed = red
                            }
                            if (Math.abs(root.viewModel.targetColorGreen - green) > 0.0001) {
                                root.viewModel.targetColorGreen = green
                            }
                            if (Math.abs(root.viewModel.targetColorBlue - blue) > 0.0001) {
                                root.viewModel.targetColorBlue = blue
                            }
                        }
                    }

                    Label { text: root.i18n.settingsTargetRadius + ": " + Number(root.viewModel.targetRadius).toFixed(2); color: root.theme.textSecondary }
                    Slider {
                        Layout.fillWidth: true
                        from: 0.1
                        to: 1.2
                        stepSize: 0.01
                        value: root.viewModel.targetRadius
                        onValueChanged: {
                            if (pressed && Math.abs(root.viewModel.targetRadius - value) > 0.0001) {
                                root.viewModel.targetRadius = value
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 14
                color: root.theme.surface
                border.color: root.theme.border
                implicitHeight: windowSettingsColumn.implicitHeight + 28

                ColumnLayout {
                    id: windowSettingsColumn
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Label {
                        text: root.i18n.settingsWindow
                        color: root.theme.textPrimary
                        font.bold: true
                    }

                    RowLayout {
                        ColumnLayout {
                            Layout.fillWidth: true

                            Label {
                                text: root.i18n.settingsWindowFullscreen
                                color: root.theme.textSecondary
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            Label {
                                text: root.i18n.settingsWindowFullscreenHint
                                color: root.theme.textDisabled || root.theme.textSecondary
                                font.pixelSize: 11
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }

                            Switch {
                                checked: root.viewModel.startFullscreen
                                onToggled: root.viewModel.startFullscreen = checked
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 14
                color: root.theme.surface
                border.color: root.theme.border
                implicitHeight: keybindingsColumn.implicitHeight + 28

                ColumnLayout {
                    id: keybindingsColumn
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Label {
                        text: root.i18n.settingsKeybindings
                        color: root.theme.textPrimary
                        font.bold: true
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: root.wideLayout ? 3 : 1
                        columnSpacing: 12
                        rowSpacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: root.i18n.settingsKeybindToggleFullscreen
                                color: root.theme.textSecondary
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                model: ["F11", "F", "Alt+Return", "Ctrl+F"]
                                currentIndex: {
                                    const idx = model.indexOf(root.viewModel.keybindToggleFullscreen)
                                    return idx >= 0 ? idx : 0
                                }
                                onActivated: function(idx) {
                                    root.viewModel.keybindToggleFullscreen = model[idx]
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: root.i18n.settingsKeybindToggleFpsCounter
                                color: root.theme.textSecondary
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                model: ["F3", "F4", "F5", "F6"]
                                currentIndex: {
                                    const idx = model.indexOf(root.viewModel.keybindToggleFpsCounter)
                                    return idx >= 0 ? idx : 0
                                }
                                onActivated: function(idx) {
                                    root.viewModel.keybindToggleFpsCounter = model[idx]
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: root.i18n.settingsKeybindToggleCrosshair
                                color: root.theme.textSecondary
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                            }
                            ComboBox {
                                Layout.fillWidth: true
                                model: ["F2", "F1", "F4", "C"]
                                currentIndex: {
                                    const idx = model.indexOf(root.viewModel.keybindToggleCrosshair)
                                    return idx >= 0 ? idx : 0
                                }
                                onActivated: function(idx) {
                                    root.viewModel.keybindToggleCrosshair = model[idx]
                                }
                            }
                        }
                    }
                }
            }

        }
    }
}
