import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Item {
    id: root

    property var theme
    property var i18n
    property string themeMode: "dark"
    property var accentColor
    property string currentScreen: "menu"

    signal screenSelected(string screen)

    implicitHeight: 40

    Material.theme: root.themeMode === "light" ? Material.Light : Material.Dark
    Material.accent: root.accentColor

    RowLayout {
        anchors.fill: parent
        spacing: 8

        Button {
            id: menuTab
            Layout.fillWidth: true
            text: root.i18n.tabMenu
            highlighted: root.currentScreen === "menu"
            hoverEnabled: true
            scale: menuTab.hovered ? 1.01 : 1.0
            onClicked: root.screenSelected("menu")

            Behavior on scale {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutCubic
                }
            }

            background: Rectangle {
                radius: 10
                color: menuTab.highlighted
                    ? root.theme.accentSoft
                    : (menuTab.down || menuTab.hovered ? root.theme.card : root.theme.surface)
                border.color: menuTab.highlighted
                    ? root.theme.accent
                    : (menuTab.hovered ? root.theme.accentSoft : root.theme.border)

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
            }

            contentItem: Label {
                text: menuTab.text
                color: menuTab.highlighted ? root.theme.accentText : root.theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: menuTab.highlighted
            }
        }

        Button {
            id: modesTab
            Layout.fillWidth: true
            text: root.i18n.tabModes
            highlighted: root.currentScreen === "modes"
            hoverEnabled: true
            scale: modesTab.hovered ? 1.01 : 1.0
            onClicked: root.screenSelected("modes")

            Behavior on scale {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutCubic
                }
            }

            background: Rectangle {
                radius: 10
                color: modesTab.highlighted
                    ? root.theme.accentSoft
                    : (modesTab.down || modesTab.hovered ? root.theme.card : root.theme.surface)
                border.color: modesTab.highlighted
                    ? root.theme.accent
                    : (modesTab.hovered ? root.theme.accentSoft : root.theme.border)

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
            }

            contentItem: Label {
                text: modesTab.text
                color: modesTab.highlighted ? root.theme.accentText : root.theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: modesTab.highlighted
            }
        }

        Button {
            id: statsTab
            Layout.fillWidth: true
            text: root.i18n.tabStats
            highlighted: root.currentScreen === "stats"
            hoverEnabled: true
            scale: statsTab.hovered ? 1.01 : 1.0
            onClicked: root.screenSelected("stats")

            Behavior on scale {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutCubic
                }
            }

            background: Rectangle {
                radius: 10
                color: statsTab.highlighted
                    ? root.theme.accentSoft
                    : (statsTab.down || statsTab.hovered ? root.theme.card : root.theme.surface)
                border.color: statsTab.highlighted
                    ? root.theme.accent
                    : (statsTab.hovered ? root.theme.accentSoft : root.theme.border)

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
            }

            contentItem: Label {
                text: statsTab.text
                color: statsTab.highlighted ? root.theme.accentText : root.theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: statsTab.highlighted
            }
        }

        Button {
            id: settingsTab
            Layout.fillWidth: true
            text: root.i18n.tabSettings
            highlighted: root.currentScreen === "settings"
            hoverEnabled: true
            scale: settingsTab.hovered ? 1.01 : 1.0
            onClicked: root.screenSelected("settings")

            Behavior on scale {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutCubic
                }
            }

            background: Rectangle {
                radius: 10
                color: settingsTab.highlighted
                    ? root.theme.accentSoft
                    : (settingsTab.down || settingsTab.hovered ? root.theme.card : root.theme.surface)
                border.color: settingsTab.highlighted
                    ? root.theme.accent
                    : (settingsTab.hovered ? root.theme.accentSoft : root.theme.border)

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
            }

            contentItem: Label {
                text: settingsTab.text
                color: settingsTab.highlighted ? root.theme.accentText : root.theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: settingsTab.highlighted
            }
        }

        Button {
            id: aboutTab
            Layout.fillWidth: true
            text: root.i18n.tabAbout
            highlighted: root.currentScreen === "about"
            hoverEnabled: true
            scale: aboutTab.hovered ? 1.01 : 1.0
            onClicked: root.screenSelected("about")

            Behavior on scale {
                NumberAnimation {
                    duration: 120
                    easing.type: Easing.OutCubic
                }
            }

            background: Rectangle {
                radius: 10
                color: aboutTab.highlighted
                    ? root.theme.accentSoft
                    : (aboutTab.down || aboutTab.hovered ? root.theme.card : root.theme.surface)
                border.color: aboutTab.highlighted
                    ? root.theme.accent
                    : (aboutTab.hovered ? root.theme.accentSoft : root.theme.border)

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
            }

            contentItem: Label {
                text: aboutTab.text
                color: aboutTab.highlighted ? root.theme.accentText : root.theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.bold: aboutTab.highlighted
            }
        }
    }
}
