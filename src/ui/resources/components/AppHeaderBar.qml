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

    signal startRequested()

    implicitHeight: 92

    Material.theme: root.themeMode === "light" ? Material.Light : Material.Dark
    Material.accent: root.accentColor

    Rectangle {
        anchors.fill: parent
        radius: 14
        color: root.theme.surface
        border.color: root.theme.border

        RowLayout {
            anchors.fill: parent
            anchors.margins: 14

            ColumnLayout {
                spacing: 2

                Label {
                    text: root.i18n.appTitle
                    color: root.theme.textPrimary
                    font.pixelSize: 26
                    font.bold: true
                }

                Label {
                    text: root.i18n.appSubtitle
                    color: root.theme.textSecondary
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: startButton
                text: root.i18n.menuActionStart
                highlighted: true
                hoverEnabled: true
                scale: startButton.hovered ? 1.02 : 1.0
                onClicked: root.startRequested()

                Behavior on scale {
                    NumberAnimation {
                        duration: 120
                        easing.type: Easing.OutCubic
                    }
                }

                background: Rectangle {
                    radius: 10
                    color: startButton.down || startButton.hovered
                        ? root.theme.accentSoft
                        : root.theme.accent
                    border.color: root.theme.accent

                    Behavior on color {
                        ColorAnimation {
                            duration: 140
                            easing.type: Easing.OutCubic
                        }
                    }
                }

                contentItem: Label {
                    text: startButton.text
                    color: root.theme.accentText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.bold: true
                }
            }
        }
    }
}
