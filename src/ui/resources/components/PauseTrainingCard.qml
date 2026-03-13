import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Rectangle {
    id: root

    required property var theme
    required property var i18n
    required property var viewModel
    property bool settingsModalVisible: false

    readonly property bool isLightTheme: !!(root.viewModel && root.viewModel.themeMode === "light")

    Material.theme: root.isLightTheme ? Material.Light : Material.Dark
    Material.accent: root.theme.accent
    Material.primary: root.theme.accent
    Material.background: root.theme.surface
    Material.foreground: root.theme.textPrimary

    anchors.centerIn: parent
    width: settingsModalVisible ? 560 : 440
    height: settingsModalVisible ? 620 : 260
    radius: 14
    color: theme.surface
    border.color: theme.border
    visible: !!(root.viewModel && root.viewModel.pauseMenuVisible)

    onVisibleChanged: {
        if (!visible) {
            settingsModalVisible = false
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Label {
            text: root.i18n.trainingPausedTitle
            color: root.theme.textPrimary
            font.pixelSize: 22
            font.bold: true
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            text: root.i18n.trainingPausedSubtitle
            color: root.theme.textSecondary
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        SettingsPanel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.settingsModalVisible
            theme: root.theme
            i18n: root.i18n
            viewModel: root.viewModel
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                Layout.fillWidth: true
                text: root.settingsModalVisible ? root.i18n.trainingBack : root.i18n.trainingOpenSettings
                hoverEnabled: true
                scale: hovered ? 1.02 : 1.0
                onClicked: {
                    root.settingsModalVisible = !root.settingsModalVisible
                }

                Behavior on scale {
                    NumberAnimation {
                        duration: 120
                        easing.type: Easing.OutCubic
                    }
                }
            }

            Button {
                Layout.fillWidth: true
                text: root.i18n.trainingContinue
                highlighted: true
                hoverEnabled: true
                scale: hovered ? 1.02 : 1.0
                onClicked: {
                    root.settingsModalVisible = false
                    root.viewModel.RequestContinueTraining()
                }

                Behavior on scale {
                    NumberAnimation {
                        duration: 120
                        easing.type: Easing.OutCubic
                    }
                }
            }

            Button {
                Layout.fillWidth: true
                text: root.i18n.trainingExit
                hoverEnabled: true
                scale: hovered ? 1.02 : 1.0
                onClicked: {
                    root.settingsModalVisible = false
                    root.viewModel.RequestExitTraining()
                }

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
