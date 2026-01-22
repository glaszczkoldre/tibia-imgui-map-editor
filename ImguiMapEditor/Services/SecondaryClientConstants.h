#pragma once
#include <cstdint>
#include <functional>

namespace MapEditor::Services {

// Forward declaration
class SecondaryClientData;

/// Sprite ID offset for secondary client sprites.
/// Secondary client sprites are loaded with IDs: original_id +
/// SECONDARY_SPRITE_OFFSET This ensures no collision with primary client sprite
/// IDs. Current Tibia sprite counts are ~100k-200k, so 1 million provides ample
/// headroom.
constexpr uint32_t SECONDARY_SPRITE_OFFSET = 1'000'000;

/// Check if a sprite ID belongs to the secondary client.
inline constexpr bool isSecondarySpriteId(uint32_t sprite_id) {
  return sprite_id >= SECONDARY_SPRITE_OFFSET;
}

/// Convert a secondary sprite ID back to its base ID.
inline constexpr uint32_t getBaseSpriteId(uint32_t sprite_id) {
  return sprite_id >= SECONDARY_SPRITE_OFFSET
             ? sprite_id - SECONDARY_SPRITE_OFFSET
             : sprite_id;
}

/// Convert a base sprite ID to a secondary sprite ID.
inline constexpr uint32_t toSecondarySpriteId(uint32_t base_id) {
  return base_id + SECONDARY_SPRITE_OFFSET;
}

/// Provider callback type for safe SecondaryClientData access.
/// Returns nullptr if secondary client is not loaded.
using SecondaryClientProvider = std::function<SecondaryClientData *()>;

/**
 * Lightweight handle that queries provider on each access.
 * Eliminates dangling pointer risk - always returns current state.
 * Provides operator-> for seamless usage with existing code.
 */
class SecondaryClientHandle {
public:
  SecondaryClientHandle() = default;
  explicit SecondaryClientHandle(SecondaryClientProvider provider)
      : provider_(std::move(provider)) {}

  void setProvider(SecondaryClientProvider provider) {
    provider_ = std::move(provider);
  }

  SecondaryClientData *get() const { return provider_ ? provider_() : nullptr; }

  SecondaryClientData *operator->() const { return get(); }
  explicit operator bool() const { return get() != nullptr; }

private:
  SecondaryClientProvider provider_;
};

} // namespace MapEditor::Services
