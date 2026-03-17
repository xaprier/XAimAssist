import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    required property var theme
    required property var i18n

    readonly property string githubUrl: String(root.i18n.aboutGithubUrl || "")
    readonly property string appName: Qt.application.name && Qt.application.name.length > 0
                                    ? Qt.application.name
                                    : String(root.i18n.appTitle || "XAimAssist")
    readonly property string appVersion: Qt.application.version && Qt.application.version.length > 0
                                       ? Qt.application.version
                                       : String(root.i18n.aboutVersionUnknown || "-")

    Rectangle {
        anchors.fill: parent
        radius: 14
        color: root.theme.surface
        border.color: root.theme.border
        border.width: 1
        ScrollView {
            id: aboutScroll
            anchors.fill: parent
            anchors.margins: 16
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: aboutScroll.availableWidth
                spacing: 12

                Label {
                    Layout.fillWidth: true
                    text: root.i18n.aboutTitle
                    color: root.theme.textPrimary
                    font.pixelSize: 22
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: root.theme.border
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 12
                    color: root.theme.card
                    border.color: root.theme.border
                    implicitHeight: appHeaderLayout.implicitHeight + 24

                    ColumnLayout {
                        id: appHeaderLayout
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        Image {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 112
                            Layout.preferredHeight: 112
                            source: "qrc:/xaimassist/ui/XAimAssist.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: root.appName
                            color: root.theme.textPrimary
                            font.pixelSize: 24
                            font.bold: true
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: root.i18n.aboutVersion + ": " + root.appVersion
                            color: root.theme.textSecondary
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        text: root.i18n.aboutSummaryTitle
                        color: root.theme.textPrimary
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.i18n.aboutSummaryBody
                        color: root.theme.textSecondary
                        wrapMode: Text.WordWrap
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        text: root.i18n.aboutHighlightsTitle
                        color: root.theme.textPrimary
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "• " + root.i18n.aboutFeatureModes
                        color: root.theme.textSecondary
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "• " + root.i18n.aboutFeatureMetrics
                        color: root.theme.textSecondary
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "• " + root.i18n.aboutFeatureStats
                        color: root.theme.textSecondary
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "• " + root.i18n.aboutFeatureInput
                        color: root.theme.textSecondary
                        wrapMode: Text.WordWrap
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "• " + root.i18n.aboutFeatureTheme
                        color: root.theme.textSecondary
                        wrapMode: Text.WordWrap
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        text: root.i18n.aboutBuiltWithTitle
                        color: root.theme.textPrimary
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.i18n.aboutBuiltWithValue
                        color: root.theme.textSecondary
                        wrapMode: Text.WordWrap
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: root.i18n.aboutDeveloperTitle
                        color: root.theme.textPrimary
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.i18n.aboutDeveloperBody
                        color: root.theme.textSecondary
                        wrapMode: Text.WordWrap
                    }

                    Button {
                        id: githubButton
                        text: root.i18n.aboutGithub
                        enabled: root.githubUrl.length > 0
                        hoverEnabled: true
                        onClicked: Qt.openUrlExternally(root.githubUrl)

                        background: Rectangle {
                            radius: 8
                            color: githubButton.down || githubButton.hovered
                                ? root.theme.accentSoft
                                : root.theme.card
                            border.color: root.theme.border
                        }

                        contentItem: Label {
                            text: githubButton.text
                            color: root.theme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.bold: true
                        }
                    }
                }
            }
        }
    }
}
