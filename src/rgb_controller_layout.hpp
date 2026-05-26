#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace rgb_controller {

struct ColorRgb {
    uint8_t r{};
    uint8_t g{};
    uint8_t b{};
};

constexpr std::size_t kTopCount   = 31;
constexpr std::size_t kRightCount = 17;
constexpr std::size_t kLeftCount  = 17;
constexpr std::size_t kLedCount   = kTopCount + kRightCount + kLeftCount;

using LedFrame = std::array<ColorRgb, kLedCount>;

} // namespace rgb_controller
