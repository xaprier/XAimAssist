/**
 * @file RandomEngineFactory.hpp
 * @brief Utility for creating well-seeded Mersenne Twister engines.
 */

#ifndef RANDOMENGINEFACTORY_HPP
#define RANDOMENGINEFACTORY_HPP

#include <random>

namespace xaimassist::gameplay {

/**
 * @brief Create an mt19937 seeded from hardware entropy + high-res clock.
 *
 * Combines multiple sources of randomness (std::random_device and
 * high-resolution clock) to ensure a well-seeded PRNG.
 *
 * @return A seeded std::mt19937 random engine
 */
std::mt19937 CreateSeededRandomEngine();

}  // namespace xaimassist::gameplay

#endif  // RANDOMENGINEFACTORY_HPP
