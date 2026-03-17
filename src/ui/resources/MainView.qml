import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import "components"

Item {
    id: root
    anchors.fill: parent

    property var uiModel: viewModel
    property var theme: viewModel.theme
    property var i18n: viewModel.i18n
    property string themeMode: viewModel.themeMode
    property var accentColor: theme.accent

    Material.theme: viewModel.themeMode === "light" ? Material.Light : Material.Dark
    Material.accent: theme.accent

    function screenIndex(screen) {
        if (screen === "menu") return 0
        if (screen === "modes") return 1
        if (screen === "training") return 2
        if (screen === "stats") return 3
        if (screen === "settings") return 4
        if (screen === "about") return 5
        return 0
    }

    function fmtSeconds(value) {
        return Number(value || 0).toFixed(1) + " " + i18n.unitSec
    }

    function fmtMs(value) {
        return Number(value || 0).toFixed(1) + " " + i18n.unitMs
    }

    function fmtPct(value) {
        return Number(value || 0).toFixed(1) + i18n.unitPercent
    }

    function fmtInt(value) {
        return Number(value || 0).toFixed(0)
    }

    Rectangle {
        anchors.fill: parent
        color: theme.window
        visible: !viewModel.sceneVisible
    }

    Item {
        anchors.fill: parent
        visible: !viewModel.sceneVisible

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            AppHeaderBar {
                Layout.fillWidth: true
                theme: root.theme
                i18n: root.i18n
                themeMode: root.themeMode
                accentColor: root.accentColor
                onStartRequested: viewModel.RequestStartSelectedMode()
            }

            NavigationTabs {
                Layout.fillWidth: true
                theme: root.theme
                i18n: root.i18n
                themeMode: root.themeMode
                accentColor: root.accentColor
                currentScreen: viewModel.currentScreen
                onScreenSelected: function(screen) {
                    viewModel.currentScreen = screen
                }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: {
                    const idx = screenIndex(viewModel.currentScreen)
                    return idx === 2 ? 0 : idx
                }

                Item {
                    MenuOverviewPanel {
                        anchors.fill: parent
                        theme: root.theme
                        i18n: root.i18n
                        uiModel: viewModel
                    }
                }

                Item {
                    ModeSelectionPanel {
                        anchors.fill: parent
                        theme: root.theme
                        i18n: root.i18n
                        viewModel: root.uiModel
                    }
                }

                Item {
                    Rectangle {
                        anchors.fill: parent
                        radius: 14
                        color: theme.surface
                        border.color: theme.border

                        Column {
                            anchors.centerIn: parent
                            spacing: 8

                            Label {
                                text: i18n.trainingTitle
                                color: theme.textPrimary
                                font.pixelSize: 22
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                width: 420
                            }
                            
                            Label {
                                text: i18n.trainingWaitForClick
                                color: theme.textSecondary
                                horizontalAlignment: Text.AlignHCenter
                                width: 420
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                Item {
                    StatisticsPanel {
                        anchors.fill: parent
                        theme: root.theme
                        i18n: root.i18n
                        viewModel: root.uiModel
                    }
                }

                Item {
                    SettingsPanel {
                        anchors.fill: parent
                        theme: root.theme
                        i18n: root.i18n
                        viewModel: root.uiModel
                    }
                }

                Item {
                    About {
                        anchors.fill: parent
                        theme: root.theme
                        i18n: root.i18n
                    }
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: viewModel.sceneVisible

        TrainingHudBar {
            theme: root.theme
            i18n: root.i18n
            uiModel: root.uiModel
        }

        FpsCounterOverlay {
            theme: root.theme
            uiModel: root.uiModel
        }

        TrainingCrosshairOverlay {
            theme: root.theme
            uiModel: root.uiModel
        }

        TrainingStateOverlays {
            theme: root.theme
            i18n: root.i18n
            uiModel: root.uiModel
        }

        PauseTrainingCard {
            theme: root.theme
            i18n: root.i18n
            viewModel: root.uiModel
        }
    }
}