import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var theme
    property var i18n
    property var uiModel

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 132
            radius: 14
            color: root.theme.card
            border.color: root.theme.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 6

                Label {
                    text: root.i18n.menuTitle
                    color: root.theme.textPrimary
                    font.pixelSize: 20
                    font.bold: true
                }

                Label {
                    text: root.i18n.menuSubtitle
                    color: root.theme.textSecondary
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        ResultCard {
            Layout.fillWidth: true
            theme: root.theme
            i18n: root.i18n
            result: root.uiModel ? root.uiModel.latestResult : ({})
            selectedModeId: root.uiModel ? root.uiModel.selectedModeId : ""
            liveStats: root.uiModel ? root.uiModel.realtimeStats : ({})
        }

        Item { Layout.fillHeight: true }
    }
}
