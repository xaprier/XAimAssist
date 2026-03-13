/// @file RandomEngineFactory.cpp
#include "gameplay/RandomEngineFactory.hpp"

#include <array>
#include <chrono>
#include <cstdint>

namespace xaimassist::gameplay {

std::mt19937 CreateSeededRandomEngine() {
    std::random_device randomDevice;

    const auto nowTicks = static_cast<std::uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());

    // Use 8 32-bit values: 6 from random_device + 2 from clock
    std::array<std::uint32_t, 8> seedData = {
        randomDevice(),
        randomDevice(),
        randomDevice(),
        randomDevice(),
        randomDevice(),
        randomDevice(),
        static_cast<std::uint32_t>(nowTicks & 0xFFFFFFFFULL),
        static_cast<std::uint32_t>((nowTicks >> 32) & 0xFFFFFFFFULL),
    };

    std::seed_seq seedSequence(seedData.begin(), seedData.end());
    return std::mt19937(seedSequence);
}

}  // namespace xaimassist::gameplay
