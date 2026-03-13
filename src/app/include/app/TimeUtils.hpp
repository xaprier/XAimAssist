/**
 * @file TimeUtils.hpp
 * @brief Time measurement utilities.
 */

#ifndef TIME_UTILS_HPP
#define TIME_UTILS_HPP

#include <chrono>

namespace xaimassist::app {

/**
 * @namespace TimeUtils
 * @brief Simple time calculation helpers.
 */
namespace TimeUtils {

/**
 * @brief Calculate elapsed milliseconds between two time points.
 * @param start Start time point
 * @param end End time point
 * @return Elapsed time in milliseconds (double precision)
 */
inline double ElapsedMilliseconds(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

}  // namespace TimeUtils
}  // namespace xaimassist::app

#endif  // TIME_UTILS_HPP
