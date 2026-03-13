import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var theme
    property var i18n
    property var viewModel

    function fmtMs(value) {
        return Number(value || 0).toFixed(1) + " " + root.i18n.unitMs
    }

    function fmtPct(value) {
        return Number(value || 0).toFixed(1) + root.i18n.unitPercent
    }

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
                Label { text: root.i18n.statsTitle; color: root.theme.textPrimary; font.pixelSize: 18; font.bold: true }
                Label { text: root.i18n.statsSubtitle; color: root.theme.textSecondary; wrapMode: Text.WordWrap }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            radius: 14
            color: root.theme.surface
            border.color: root.theme.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 6

                Label { text: root.i18n.statsBestSession; color: root.theme.textPrimary; font.bold: true }

                readonly property bool hasBestSession: Object.keys(root.viewModel.bestSession).length > 0

                Label {
                    visible: !parent.hasBestSession
                    text: root.i18n.statsNoBest
                    color: root.theme.textSecondary
                }

                GridLayout {
                    visible: parent.hasBestSession
                    columns: 2
                    rowSpacing: 4
                    columnSpacing: 10

                    Label { text: root.i18n.statsScore; color: root.theme.textSecondary }
                    Label { text: root.fmtInt(root.viewModel.bestSession.score); color: root.theme.textPrimary; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }

                    Label { text: root.i18n.statsAccuracy; color: root.theme.textSecondary }
                    Label { text: root.fmtPct(root.viewModel.bestSession.accuracyPercent); color: root.theme.textPrimary; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }

                    Label { text: root.i18n.statsReaction; color: root.theme.textSecondary }
                    Label { text: root.fmtMs(root.viewModel.bestSession.averageReactionTimeMs); color: root.theme.textPrimary; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 14
            color: root.theme.surface
            border.color: root.theme.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Label { text: root.i18n.statsRecent; color: root.theme.textPrimary; font.bold: true }

                Label {
                    visible: root.viewModel.recentSessions.length === 0
                    text: root.i18n.statsNoRecent
                    color: root.theme.textSecondary
                }

                ListView {
                    visible: root.viewModel.recentSessions.length > 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 8
                    model: root.viewModel.recentSessions

                    delegate: Rectangle {
                        required property var modelData
                        width: ListView.view.width
                        height: 92
                        radius: 10
                        color: root.theme.card
                        border.color: root.theme.border

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            columns: 2
                            rowSpacing: 3
                            columnSpacing: 10

                            Label { text: modelData.modeId; color: root.theme.textPrimary; font.bold: true }
                            Label { text: modelData.createdAt; color: root.theme.textSecondary; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }

                            Label { text: root.i18n.statsScore + ": " + root.fmtInt(modelData.score); color: root.theme.textSecondary }
                            Label { text: root.i18n.statsAccuracy + ": " + root.fmtPct(modelData.accuracyPercent); color: root.theme.textSecondary; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }

                            Label { text: root.i18n.statsShots + ": " + root.fmtInt(modelData.shotsFired); color: root.theme.textSecondary }
                            Label { text: root.i18n.statsReaction + ": " + root.fmtMs(modelData.averageReactionTimeMs); color: root.theme.textSecondary; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }
                        }
                    }
                }
            }
        }
    }
}