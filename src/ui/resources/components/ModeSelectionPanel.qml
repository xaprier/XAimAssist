import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var theme
    property var i18n
    property var viewModel

    function fmtInt(value) {
        return Number(value || 0).toFixed(0)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 84
            radius: 14
            color: root.theme.card
            border.color: root.theme.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 4
                Label { text: root.i18n.modeTitle; color: root.theme.textPrimary; font.pixelSize: 18; font.bold: true }
                Label { text: root.i18n.modeSubtitle; color: root.theme.textSecondary; wrapMode: Text.WordWrap }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                radius: 14
                color: root.theme.surface
                border.color: root.theme.border

                ListView {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8
                    clip: true
                    model: root.viewModel.modes

                    delegate: Rectangle {
                        required property var modelData
                        width: ListView.view.width
                        implicitHeight: modeCardColumn.implicitHeight + 20
                        height: Math.max(112, implicitHeight)
                        radius: 10
                        color: modelData.id === root.viewModel.selectedModeId
                            ? root.theme.surface : root.theme.card
                        border.color: modelData.id === root.viewModel.selectedModeId
                            ? root.theme.accent
                            : (modeHover.hovered ? root.theme.accentSoft : root.theme.border)

                        Behavior on color {
                            ColorAnimation {
                                duration: 140
                                easing.type: Easing.OutCubic
                            }
                        }

                        Behavior on border.color {
                            ColorAnimation {
                                duration: 140
                                easing.type: Easing.OutCubic
                            }
                        }

                        HoverHandler {
                            id: modeHover
                        }

                        TapHandler {
                            onTapped: root.viewModel.selectedModeId = modelData.id
                        }

                        ColumnLayout {
                            id: modeCardColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Label {
                                text: modelData.name
                                color: root.theme.textPrimary
                                font.pixelSize: 16
                                font.bold: true
                                Layout.fillWidth: true
                            }

                            Label {
                                text: modelData.description
                                color: root.theme.textSecondary
                                wrapMode: Text.WordWrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Label {
                                text: root.i18n.modeDuration + ": " + modelData.durationLabel + " • " + root.i18n.modeDistance + ": " + modelData.distanceLabel
                                color: root.theme.textSecondary
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                radius: 14
                color: root.theme.surface
                border.color: root.theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 6

                    Label {
                        text: root.i18n.modeConfiguredDuration + ": " + root.fmtInt(root.viewModel.modeDurationSeconds) + " " + root.i18n.unitSec
                        color: root.theme.textSecondary
                    }

                    Slider {
                        Layout.fillWidth: true
                        from: 10
                        to: 600
                        stepSize: 5
                        value: root.viewModel.modeDurationSeconds
                        onValueChanged: {
                            if (pressed && Math.abs(root.viewModel.modeDurationSeconds - value) > 0.0001) {
                                root.viewModel.modeDurationSeconds = value
                            }
                        }
                    }

                    Label {
                        text: root.i18n.modeConfiguredDistance + ": " + Number(root.viewModel.modeTargetDistance).toFixed(1)
                        color: root.theme.textSecondary
                    }

                    Slider {
                        Layout.fillWidth: true
                        from: 1
                        to: 150
                        stepSize: 0.5
                        value: root.viewModel.modeTargetDistance
                        onValueChanged: {
                            if (pressed && Math.abs(root.viewModel.modeTargetDistance - value) > 0.0001) {
                                root.viewModel.modeTargetDistance = value
                            }
                        }
                    }

                    Loader {
                        active: root.viewModel.modeSettings.length > 0
                        visible: active
                        Layout.fillWidth: true
                        Layout.preferredHeight: active && item ? item.implicitHeight : 0
                        sourceComponent: Item {
                            implicitHeight: settingsColumn.implicitHeight
                            width: parent ? parent.width : implicitWidth

                            ColumnLayout {
                                id: settingsColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                spacing: 8

                                Repeater {
                                    model: root.viewModel.modeSettings

                                    delegate: ColumnLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        spacing: 4

                                        Label {
                                            text: modelData.name + ": " + (modelData.integerOnly
                                                  ? root.fmtInt(modelData.value)
                                                  : Number(modelData.value).toFixed(2))
                                            color: root.theme.textSecondary
                                        }

                                        Slider {
                                            Layout.fillWidth: true
                                            from: modelData.minValue
                                            to: modelData.maxValue
                                            stepSize: modelData.step
                                            value: modelData.value
                                            onValueChanged: {
                                                if (pressed && Math.abs(modelData.value - value) > 0.0001) {
                                                    root.viewModel.SetModeSettingValue(
                                                        modelData.id,
                                                        modelData.integerOnly ? Math.round(value) : value)
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    Button {
                        Layout.fillWidth: true
                        text: root.i18n.modeReset
                        hoverEnabled: true
                        onClicked: root.viewModel.ResetSelectedModeSettingsToDefaults()
                    }

                    Button {
                        Layout.fillWidth: true
                        text: root.i18n.modeStart
                        highlighted: true
                        hoverEnabled: true
                        scale: hovered ? 1.02 : 1.0
                        onClicked: root.viewModel.RequestStartSelectedMode()

                        Behavior on scale {
                            NumberAnimation {
                                duration: 120
                                easing.type: Easing.OutCubic
                            }
                        }
                    }
                }
            }
        }
    }
}