import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property var theme
    property string badgeText: ""
    property color badgeBackgroundColor: root.theme ? root.theme.card : "#2A2A2A"
    property color badgeBorderColor: root.theme ? root.theme.border : "#505050"
    property color badgeTextColor: root.theme ? root.theme.textPrimary : "#F2F2F2"
    property string tooltipTitle: ""
    property var tooltipRows: ([])

    readonly property bool hasTooltipRows: !!(tooltipRows && tooltipRows.length > 0)
    readonly property bool tooltipVisible: hasTooltipRows && (badgeHover.hovered || tooltipHover.hovered)

    implicitWidth: badgePill.implicitWidth
    implicitHeight: badgePill.implicitHeight
    z: root.tooltipVisible ? 1000 : 0

    Rectangle {
        id: badgePill
        radius: 10
        color: root.badgeBackgroundColor
        border.color: root.badgeBorderColor
        implicitWidth: badgeLabel.implicitWidth + 16
        implicitHeight: badgeLabel.implicitHeight + 8

        Label {
            id: badgeLabel
            anchors.centerIn: parent
            text: root.badgeText
            color: root.badgeTextColor
            font.bold: true
        }

        HoverHandler {
            id: badgeHover
            cursorShape: root.hasTooltipRows ? Qt.WhatsThisCursor : Qt.ArrowCursor
        }
    }

    Rectangle {
        id: tooltipBubble
        visible: root.tooltipVisible
        z: 1000
        radius: 10
        color: root.theme ? root.theme.card : "#1F1F1F"
        border.color: root.theme ? root.theme.border : "#555555"
        anchors.top: badgePill.bottom
        anchors.topMargin: 6
        anchors.right: badgePill.right
        implicitWidth: Math.max(260, tooltipContent.implicitWidth + 20)
        implicitHeight: tooltipContent.implicitHeight + 16

        HoverHandler {
            id: tooltipHover
            enabled: tooltipBubble.visible
        }

        ColumnLayout {
            id: tooltipContent
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            Label {
                visible: root.tooltipTitle.length > 0
                text: root.tooltipTitle
                color: root.theme ? root.theme.textPrimary : "#F2F2F2"
                font.bold: true
            }

            Repeater {
                model: root.tooltipRows || []

                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        text: modelData.label
                        color: root.theme ? root.theme.textSecondary : "#C8C8C8"
                        Layout.fillWidth: true
                    }

                    Label {
                        text: modelData.value
                        color: root.theme ? root.theme.textPrimary : "#F2F2F2"
                        font.bold: true
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }
    }
}