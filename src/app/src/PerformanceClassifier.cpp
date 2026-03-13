/// @file PerformanceClassifier.cpp
#include "app/PerformanceClassifier.hpp"

namespace xaimassist::app::PerformanceClassifier {

namespace {

/// Convert tier string to numeric score for weighted calculations.
double TierScore(const std::string& tier) {
    if (tier == "best_result") {
        return 2.0;
    }
    if (tier == "above_average") {
        return 1.0;
    }
    if (tier == "below_average") {
        return -1.0;
    }
    if (tier == "worst_result") {
        return -2.0;
    }
    return 0.0;  // "average" or "first_result"
}

}  // anonymous namespace

double BestValueForDirection(
    const persistence::ModeMetricAggregate& aggregate,
    MetricDirection direction) {
    return direction == MetricDirection::HigherIsBetter
               ? aggregate.maximum
               : aggregate.minimum;
}

double WorstValueForDirection(
    const persistence::ModeMetricAggregate& aggregate,
    MetricDirection direction) {
    return direction == MetricDirection::HigherIsBetter
               ? aggregate.minimum
               : aggregate.maximum;
}

std::string ClassifyMetricTier(
    std::uint64_t sessionCount,
    double value,
    double averageValue,
    double bestValue,
    double worstValue,
    MetricDirection direction) {
    // First session has no comparison
    if (sessionCount <= 1) {
        return "first_result";
    }

    if (direction == MetricDirection::HigherIsBetter) {
        // For metrics where higher is better
        if (value >= bestValue - COMPARISON_EPSILON) {
            return "best_result";
        }
        if (value <= worstValue + COMPARISON_EPSILON) {
            return "worst_result";
        }
        if (value > averageValue + COMPARISON_EPSILON) {
            return "above_average";
        }
        if (value + COMPARISON_EPSILON < averageValue) {
            return "below_average";
        }
        return "average";
    }

    // For metrics where lower is better (e.g. reaction time)
    if (value <= bestValue + COMPARISON_EPSILON) {
        return "best_result";
    }
    if (value >= worstValue - COMPARISON_EPSILON) {
        return "worst_result";
    }
    if (value + COMPARISON_EPSILON < averageValue) {
        return "above_average";
    }
    if (value > averageValue + COMPARISON_EPSILON) {
        return "below_average";
    }
    return "average";
}

std::string ClassifyOverallPerformanceTier(
    std::uint64_t sessionCount,
    const std::vector<EvaluatedMetricComparison>& metricComparisons) {
    // First session
    if (sessionCount <= 1) {
        return "first_result";
    }

    // No metrics to evaluate
    if (metricComparisons.empty()) {
        return "average";
    }

    // Calculate weighted average of tier scores
    double weightedSum = 0.0;
    double totalWeight = 0.0;

    for (const auto& comparison : metricComparisons) {
        if (comparison.weight <= 0.0) {
            continue;
        }
        weightedSum += comparison.weight * TierScore(comparison.tier);
        totalWeight += comparison.weight;
    }

    if (totalWeight <= 0.0) {
        return "average";
    }

    // Normalize and classify
    const double normalizedScore = weightedSum / totalWeight;

    // Thresholds for tier classification
    if (normalizedScore >= 1.4) {
        return "best_result";
    }
    if (normalizedScore <= -1.4) {
        return "worst_result";
    }
    if (normalizedScore >= 0.35) {
        return "above_average";
    }
    if (normalizedScore <= -0.35) {
        return "below_average";
    }

    return "average";
}

}  // namespace xaimassist::app::PerformanceClassifier
