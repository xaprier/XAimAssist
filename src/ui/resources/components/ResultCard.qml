import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var theme
    property var i18n
    property var result: ({})
    property string selectedModeId: ""
    property var liveStats: ({})

    radius: 14
    color: root.theme.surface
    border.color: root.theme.border
    implicitHeight: contentColumn.implicitHeight + 28

    function hasResult() {
        return !!(root.result && typeof root.result === "object" && Object.keys(root.result).length > 0)
    }

    function fmtSeconds(value) {
        return Number(value || 0).toFixed(1) + " " + root.i18n.unitSec
    }

    function fmtMs(value) {
        return Number(value || 0).toFixed(1) + " " + root.i18n.unitMs
    }

    function fmtPct(value) {
        return Number(value || 0).toFixed(1) + root.i18n.unitPercent
    }

    function fmtInt(value) {
        return Number(value || 0).toFixed(0)
    }

    function fmtScore(value) {
        return Number(value || 0).toFixed(1)
    }

    function stopReasonLabel() {
        return root.result.stopReason === "aborted"
            ? root.i18n.resultStopAborted
            : root.i18n.resultStopCompleted
    }

    function statusText() {
        if (!hasResult()) {
            return root.i18n.resultLive
        }

        const state = resultState()
        if (state === "invalid") {
            return root.i18n.resultInvalid
        }

        if (state === "saved" || state === "pending") {
            return root.i18n.resultSaved
        }

        return root.i18n.resultNotSaved
    }

    function resultState() {
        if (!hasResult()) {
            return "live"
        }

        const rawState = String(root.result.saveState || "")
        if (rawState.length > 0) {
            return rawState
        }

        return root.result.wasSaved ? "saved" : "not_saved"
    }

    function invalidReasonText() {
        const code = String(root.result.invalidReasonCode || "")
        if (code === "reaction_time_zero") return root.i18n.resultInvalidReasonReaction
        if (code === "hits_zero") return root.i18n.resultInvalidReasonHits
        if (code === "score_negative") return root.i18n.resultInvalidReasonScore
        if (code === "accuracy_extreme") return root.i18n.resultInvalidReasonAccuracy
        return root.i18n.resultInvalidReasonUnknown
    }

    function statusBackgroundColor() {
        if (!hasResult()) {
            return root.theme.card
        }

        const state = resultState()
        if (state === "saved" || state === "pending") {
            return root.theme.accentSoft
        }

        return root.theme.danger
    }

    function statusBorderColor() {
        if (!hasResult()) {
            return root.theme.border
        }

        const state = resultState()
        if (state === "saved" || state === "pending") {
            return root.theme.accent
        }

        return root.theme.border
    }

    function statusTextColor() {
        if (!hasResult()) {
            return root.theme.textSecondary
        }

        const state = resultState()
        if (state === "saved" || state === "pending") {
            return root.theme.accentText
        }

        return root.theme.textPrimary
    }

    function hasBenchmark() {
        if (!hasResult() || resultState() !== "saved") {
            return false
        }

        const tier = String(root.result.performanceTier || "")
        return tier.length > 0 && tier !== "pending" && tier !== "unavailable"
    }

    function benchmarkTier() {
        if (!hasResult() || resultState() !== "saved") {
            return "unavailable"
        }

        return String(root.result.performanceTier || "unavailable")
    }

    function benchmarkText() {
        return tierText(benchmarkTier())
    }

    function tierText(tier) {
        if (tier === "first_result") return root.i18n.resultPerfFirst
        if (tier === "best_result") return root.i18n.resultPerfBest
        if (tier === "worst_result") return root.i18n.resultPerfWorst
        if (tier === "above_average") return root.i18n.resultPerfAboveAverage
        if (tier === "below_average") return root.i18n.resultPerfBelowAverage
        if (tier === "average") return root.i18n.resultPerfAverage
        return root.i18n.resultPerfUnavailable
    }

    function comparisonList() {
        if (!hasResult() || resultState() !== "saved") {
            return []
        }

        const comparisons = root.result.performanceComparisons
        if (!comparisons || typeof comparisons.length !== "number") {
            return []
        }

        return comparisons
    }

    function badgeBackgroundColorForTier(tier) {
        if (tier === "best_result" || tier === "above_average") {
            return root.theme.accentSoft
        }

        if (tier === "worst_result" || tier === "below_average") {
            return root.theme.danger
        }

        return root.theme.card
    }

    function badgeBorderColorForTier(tier) {
        if (tier === "best_result" || tier === "above_average") {
            return root.theme.accent
        }

        if (tier === "worst_result" || tier === "below_average") {
            return root.theme.border
        }

        return root.theme.border
    }

    function badgeTextColorForTier(tier) {
        if (tier === "best_result" || tier === "above_average") {
            return root.theme.accentText
        }

        if (tier === "worst_result" || tier === "below_average") {
            return root.theme.textPrimary
        }

        return root.theme.textSecondary
    }

    function benchmarkBackgroundColor() {
        return badgeBackgroundColorForTier(benchmarkTier())
    }

    function benchmarkBorderColor() {
        return badgeBorderColorForTier(benchmarkTier())
    }

    function benchmarkTextColor() {
        return badgeTextColorForTier(benchmarkTier())
    }

    function hasBenchmarkStats() {
        return hasResult()
            && resultState() === "saved"
            && Number(root.result.modeSessionCount || 0) > 0
            && comparisonList().length > 0
    }

    function comparisonForMetric(metricId) {
        if (!hasBenchmarkStats() || !metricId || metricId.length === 0) {
            return null
        }

        const comparisons = comparisonList()
        for (let index = 0; index < comparisons.length; ++index) {
            const metric = comparisons[index] || ({})
            if (String(metric.metricId || "") === metricId) {
                return metric
            }
        }

        return null
    }

    function hasMetricComparison(metricId) {
        return comparisonForMetric(metricId) !== null
    }

    function metricTier(metricId) {
        const metric = comparisonForMetric(metricId)
        if (!metric) {
            return "unavailable"
        }

        return String(metric.tier || "average")
    }

    function metricComparisonText(metricId) {
        return tierText(metricTier(metricId))
    }

    function detailModel() {
        if (!hasResult()) {
            const live = root.liveStats || ({})
            return [
                { label: root.i18n.resultMode, value: String(root.selectedModeId || "-") },
                { label: root.i18n.trainingElapsed, value: fmtSeconds(live.elapsedSeconds) },
                { label: root.i18n.trainingScore, value: fmtInt(live.score), metricId: "score" },
                { label: root.i18n.trainingHits, value: fmtInt(live.hits), metricId: "hits" },
                { label: root.i18n.trainingMisses, value: fmtInt(live.misses), metricId: "misses" },
                { label: root.i18n.trainingAccuracy, value: fmtPct(live.accuracyPercent), metricId: "accuracyPercent" },
                { label: root.i18n.trainingReaction, value: fmtMs(live.averageReactionTimeMs), metricId: "averageReactionTimeMs" }
            ]
        }

        const details = [
            { label: root.i18n.resultMode, value: String(root.result.modeId || "-") },
            { label: root.i18n.resultConfiguredDuration, value: fmtSeconds(root.result.configuredDurationSeconds) },
            { label: root.i18n.resultElapsedDuration, value: fmtSeconds(root.result.elapsedSeconds) },
            { label: root.i18n.resultStopReason, value: stopReasonLabel() },
            { label: root.i18n.resultStoppedAt, value: String(root.result.stoppedAt || "-") },
            { label: root.i18n.trainingScore, value: fmtInt(root.result.score), metricId: "score" },
            { label: root.i18n.trainingHits, value: fmtInt(root.result.hits), metricId: "hits" },
            { label: root.i18n.trainingMisses, value: fmtInt(root.result.misses), metricId: "misses" },
            { label: root.i18n.trainingAccuracy, value: fmtPct(root.result.accuracyPercent), metricId: "accuracyPercent" },
            { label: root.i18n.trainingReaction, value: fmtMs(root.result.averageReactionTimeMs), metricId: "averageReactionTimeMs" }
        ]

        return details
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        RowLayout {
            id: headerRow
            Layout.fillWidth: true
            z: benchmarkBadge.tooltipVisible ? 1000 : 0

            Label {
                text: root.i18n.resultTitle
                color: root.theme.textPrimary
                font.pixelSize: 18
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            RowLayout {
                spacing: 8

                Rectangle {
                    radius: 10
                    color: statusBackgroundColor()
                    border.color: statusBorderColor()
                    implicitWidth: statusLabel.implicitWidth + 16
                    implicitHeight: statusLabel.implicitHeight + 8

                    Label {
                        id: statusLabel
                        anchors.centerIn: parent
                        text: statusText()
                        color: statusTextColor()
                        font.bold: true
                    }
                }

                PerformanceTierBadge {
                    id: benchmarkBadge
                    visible: hasBenchmark()
                    theme: root.theme
                    badgeText: benchmarkText()
                    badgeBackgroundColor: benchmarkBackgroundColor()
                    badgeBorderColor: benchmarkBorderColor()
                    badgeTextColor: benchmarkTextColor()
                    tooltipTitle: ""
                    tooltipRows: []
                }
            }
        }

        Label {
            visible: hasResult() && resultState() === "not_saved"
            text: hasResult() ? root.i18n.resultNotSavedHint : root.i18n.resultLiveHint
            color: root.theme.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            visible: hasResult() && resultState() === "invalid"
            text: root.i18n.resultInvalidHint + " (" + invalidReasonText() + ")"
            color: root.theme.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            visible: !hasResult()
            text: root.i18n.resultLiveHint
            color: root.theme.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item {
            id: detailsWrap
            Layout.fillWidth: true
            implicitHeight: detailRows.implicitHeight

            property real chipSpacing: 8
            property real chipMinWidth: 160
            property real chipMaxWidth: 260
            property int maxColumns: 4
            property var metricItems: root.detailModel() || []
            property int columnCount: {
                const availableWidth = Math.max(0, detailsWrap.width)
                const possibleColumns = Math.floor((availableWidth + chipSpacing) / (chipMinWidth + chipSpacing))
                return Math.max(1, Math.min(maxColumns, possibleColumns))
            }
            property real chipWidth: {
                const columns = Math.max(1, columnCount)
                const availableWidth = Math.max(0, detailsWrap.width - chipSpacing * (columns - 1))
                return Math.max(chipMinWidth, Math.min(chipMaxWidth, availableWidth / columns))
            }
            function rows() {
                const chunkedRows = []
                const columns = Math.max(1, columnCount)
                const items = metricItems || []
                for (let index = 0; index < items.length; index += columns) {
                    chunkedRows.push(items.slice(index, index + columns))
                }
                return chunkedRows
            }
            property var rowModel: rows()

            Column {
                id: detailRows
                width: parent.width
                spacing: detailsWrap.chipSpacing

                Repeater {
                    model: detailsWrap.rowModel

                    delegate: Row {
                        required property var modelData
                        spacing: detailsWrap.chipSpacing
                        anchors.horizontalCenter: parent.horizontalCenter

                        Repeater {
                            model: modelData

                            delegate: InfoMetricChip {
                                required property var modelData
                                theme: root.theme
                                width: detailsWrap.chipWidth
                                labelText: modelData.label
                                valueText: modelData.value
                                readonly property string metricId: modelData.metricId
                                    ? String(modelData.metricId)
                                    : ""
                                readonly property bool showComparison: metricId.length > 0
                                    && hasMetricComparison(metricId)
                                readonly property string metricTierValue: showComparison
                                    ? metricTier(metricId)
                                    : "unavailable"
                                comparisonVisible: showComparison
                                comparisonText: showComparison
                                    ? metricComparisonText(metricId)
                                    : ""
                                comparisonBackgroundColor: badgeBackgroundColorForTier(metricTierValue)
                                comparisonBorderColor: badgeBorderColorForTier(metricTierValue)
                                comparisonTextColor: badgeTextColorForTier(metricTierValue)
                            }
                        }
                    }
                }
            }
        }
    }
}
