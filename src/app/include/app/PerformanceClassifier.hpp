/**
 * @file PerformanceClassifier.hpp
 * @brief Classifies session performance metrics into tiers.
 */

#ifndef PERFORMANCE_CLASSIFIER_HPP
#define PERFORMANCE_CLASSIFIER_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "persistence/SessionHistory.hpp"

namespace xaimassist::app {

/**
 * @namespace PerformanceClassifier
 * @brief Analyzes session metrics and classifies performance.
 *
 * Compares current session metrics against historical aggregates
 * to determine relative performance tiers (best, above average, etc.).
 */
namespace PerformanceClassifier {

/// Direction of metric improvement.
enum class MetricDirection {
    HigherIsBetter,  ///< e.g. accuracy, score
    LowerIsBetter    ///< e.g. reaction time
};

/// Evaluated metric with its tier and weight.
struct EvaluatedMetricComparison {
    std::string tier;  ///< "best_result", "above_average", etc.
    double weight{1.0};
};

/// Performance comparison epsilon for floating point comparisons.
constexpr double COMPARISON_EPSILON = 1e-6;

/**
 * @brief Get the best value from aggregate based on metric direction.
 * @param aggregate Historical metric aggregate
 * @param direction Whether higher or lower values are better
 * @return Best value (maximum if higher is better, minimum otherwise)
 */
double BestValueForDirection(const persistence::ModeMetricAggregate& aggregate, MetricDirection direction);

/**
 * @brief Get the worst value from aggregate based on metric direction.
 * @param aggregate Historical metric aggregate
 * @param direction Whether higher or lower values are better
 * @return Worst value (minimum if higher is better, maximum otherwise)
 */
double WorstValueForDirection(const persistence::ModeMetricAggregate& aggregate, MetricDirection direction);

/**
 * @brief Classify a single metric value into a performance tier.
 * @param sessionCount Total sessions for this mode
 * @param value Current metric value
 * @param averageValue Historical average
 * @param bestValue Historical best
 * @param worstValue Historical worst
 * @param direction Metric improvement direction
 * @return Tier string: "best_result", "above_average", "average",
 *         "below_average", "worst_result", or "first_result"
 */
std::string ClassifyMetricTier(
    std::uint64_t sessionCount,
    double value,
    double averageValue,
    double bestValue,
    double worstValue,
    MetricDirection direction);

/**
 * @brief Classify overall session performance from multiple metrics.
 * @param sessionCount Total sessions for this mode
 * @param metricComparisons Vector of evaluated metrics with tiers and weights
 * @return Overall tier: "best_result", "above_average", "average",
 *         "below_average", "worst_result", or "first_result"
 */
std::string ClassifyOverallPerformanceTier(std::uint64_t sessionCount, const std::vector<EvaluatedMetricComparison>& metricComparisons);

}  // namespace PerformanceClassifier
}  // namespace xaimassist::app

#endif  // PERFORMANCE_CLASSIFIER_HPP
