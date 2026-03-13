import QtQuick
import QtQuick.Controls

Item {
    id: root

    required property var theme
    required property var i18n
    required property var uiModel

    anchors.fill: parent

    Rectangle {
        anchors.centerIn: parent
        width: 520
        height: 160
        radius: 14
        color: theme.surface
        border.color: theme.border
        visible: uiModel.waitingForSceneClick

        Column {
            anchors.centerIn: parent
            spacing: 8

            Label {
                text: i18n.trainingTitle
                color: theme.textPrimary
                font.pixelSize: 24
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                width: 460
            }

            Label {
                text: i18n.trainingWaitForClick
                color: theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                width: 460
                wrapMode: Text.WordWrap
            }
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 240
        height: 240
        radius: 120
        color: theme.surface
        border.color: theme.border
        visible: uiModel.countdownVisible

        Column {
            anchors.centerIn: parent
            spacing: 4

            Label {
                text: i18n.trainingCountdown
                color: theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                width: 200
            }

            Label {
                text: String(uiModel.countdownValue)
                color: theme.textPrimary
                font.pixelSize: 88
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                width: 200
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: theme.overlayScrim
        visible: uiModel.pauseMenuVisible
    }
}
