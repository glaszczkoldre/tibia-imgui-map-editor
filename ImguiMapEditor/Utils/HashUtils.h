#pragma once

#include <cstdint>
#include <string_view>

namespace MapEditor::Utils {

constexpr uint32_t kFnvPrime = 16777619u;
constexpr uint32_t kFnvOffsetBasis = 2166136261u;

inline void mixSeed(uint32_t &seed, uint32_t value) noexcept {
  seed ^= value;
  seed *= kFnvPrime;
}

inline void mixSeed(uint32_t &seed, std::string_view value) noexcept {
  for (const auto ch : value) {
    mixSeed(seed, static_cast<uint8_t>(ch));
  }
}

} // namespace MapEditor::Utils
