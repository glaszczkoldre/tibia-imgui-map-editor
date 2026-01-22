#pragma once
/**
 * @file EnumFlags.h
 * @brief Provides bitmask operators for enum classes.
 *
 * Usage:
 *   enum class MyFlags : uint8_t { A = 1, B = 2, C = 4 };
 *   ENABLE_BITMASK_OPERATORS(MyFlags)
 *
 *   MyFlags flags = MyFlags::A | MyFlags::B;
 */

#include <type_traits>

/**
 * Macro to enable bitwise operators for an enum class.
 * This allows using |, &, ^, ~ operators on enum values.
 */
#define ENABLE_BITMASK_OPERATORS(EnumType)                                     \
  inline constexpr EnumType operator|(EnumType lhs, EnumType rhs) {            \
    using T = std::underlying_type_t<EnumType>;                                \
    return static_cast<EnumType>(static_cast<T>(lhs) | static_cast<T>(rhs));   \
  }                                                                            \
  inline constexpr EnumType operator&(EnumType lhs, EnumType rhs) {            \
    using T = std::underlying_type_t<EnumType>;                                \
    return static_cast<EnumType>(static_cast<T>(lhs) & static_cast<T>(rhs));   \
  }                                                                            \
  inline constexpr EnumType operator^(EnumType lhs, EnumType rhs) {            \
    using T = std::underlying_type_t<EnumType>;                                \
    return static_cast<EnumType>(static_cast<T>(lhs) ^ static_cast<T>(rhs));   \
  }                                                                            \
  inline constexpr EnumType operator~(EnumType val) {                          \
    using T = std::underlying_type_t<EnumType>;                                \
    return static_cast<EnumType>(~static_cast<T>(val));                        \
  }                                                                            \
  inline constexpr EnumType &operator|=(EnumType &lhs, EnumType rhs) {         \
    lhs = lhs | rhs;                                                           \
    return lhs;                                                                \
  }                                                                            \
  inline constexpr EnumType &operator&=(EnumType &lhs, EnumType rhs) {         \
    lhs = lhs & rhs;                                                           \
    return lhs;                                                                \
  }                                                                            \
  inline constexpr EnumType &operator^=(EnumType &lhs, EnumType rhs) {         \
    lhs = lhs ^ rhs;                                                           \
    return lhs;                                                                \
  }                                                                            \
  inline constexpr bool hasFlag(EnumType flags, EnumType flag) {               \
    using T = std::underlying_type_t<EnumType>;                                \
    return (static_cast<T>(flags) & static_cast<T>(flag)) ==                   \
           static_cast<T>(flag);                                               \
  }
