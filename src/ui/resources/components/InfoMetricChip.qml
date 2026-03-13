import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var theme
    property string labelText: ""
    property string valueText: ""
    property bool comparisonVisible: false
    property string comparisonText: ""
    property color comparisonBackgroundColor: root.theme ? root.theme.card : "#2A2A2A"
    property color comparisonBorderColor: root.theme ? root.theme.border : "#505050"
    property color comparisonTextColor: root.theme ? root.theme.textPrimary : "#F2F2F2"

    radius: 10
    color: root.theme.card
    border.color: root.theme.border
    implicitHeight: 84
    z: comparisonBadge.tooltipVisible ? 1000 : 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 2

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
                text: root.labelText
                color: root.theme.textSecondary
                font.pixelSize: 12
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            PerformanceTierBadge {
                id: comparisonBadge
                visible: root.comparisonVisible
                theme: root.theme
                badgeText: root.comparisonText
                badgeBackgroundColor: root.comparisonBackgroundColor
                badgeBorderColor: root.comparisonBorderColor
                badgeTextColor: root.comparisonTextColor
                tooltipTitle: ""
                tooltipRows: []
            }
        }

        Label {
            text: root.valueText
            color: root.theme.textPrimary
            font.pixelSize: 18
            font.bold: true
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
