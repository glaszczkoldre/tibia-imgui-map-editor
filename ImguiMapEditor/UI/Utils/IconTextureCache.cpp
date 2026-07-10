#include "IconTextureCache.h"

#include <filesystem>
#include <format>
#include <stdexcept>
#include <vector>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include "Rendering/Core/Texture.h"

namespace MapEditor::UI {

namespace {

void makeBackgroundTransparent(unsigned char *data, int width, int height) {
  if (!data || width <= 0 || height <= 0) return;

  std::vector<bool> visited(width * height, false);
  std::vector<std::pair<int, int>> queue;
  queue.reserve(width * height);

  // Read target background color from top-left corner
  unsigned char bgR = data[0];
  unsigned char bgG = data[1];
  unsigned char bgB = data[2];

  auto isBackgroundPixel = [&](int x, int y) {
    int idx = (y * width + x) * 4;
    unsigned char r = data[idx + 0];
    unsigned char g = data[idx + 1];
    unsigned char b = data[idx + 2];
    unsigned char a = data[idx + 3];

    if (a == 0) return false; // Already transparent
    
    // Check if it matches the corner color (grey)
    if (r == bgR && g == bgG && b == bgB) return true;
    // Check if it matches pure white (255, 255, 255)
    if (r == 255 && g == 255 && b == 255) return true;
    // Check if it matches magenta (255, 0, 255)
    if (r == 255 && g == 0 && b == 255) return true;
    
    return false;
  };

  // Add the 4 corners as start points for flood fill
  std::vector<std::pair<int, int>> starts = {
      {0, 0}, {width - 1, 0}, {0, height - 1}, {width - 1, height - 1}};

  for (const auto &start : starts) {
    int sx = start.first;
    int sy = start.second;
    if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
      if (isBackgroundPixel(sx, sy)) {
        int idx = sy * width + sx;
        if (!visited[idx]) {
          visited[idx] = true;
          queue.push_back({sx, sy});
        }
      }
    }
  }

  size_t head = 0;
  while (head < queue.size()) {
    auto [cx, cy] = queue[head++];

    // Make this pixel transparent
    int idx = (cy * width + cx) * 4;
    data[idx + 3] = 0;

    // Neighbors (4-connectivity)
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    for (int i = 0; i < 4; ++i) {
      int nx = cx + dx[i];
      int ny = cy + dy[i];
      if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
        int nidx = ny * width + nx;
        if (!visited[nidx] && isBackgroundPixel(nx, ny)) {
          visited[nidx] = true;
          queue.push_back({nx, ny});
        }
      }
    }
  }
}

} // namespace

IconTextureCache::~IconTextureCache() = default;

void IconTextureCache::loadAll(const std::string &assetsDir) {
  const std::vector<std::string> requiredIcons = {
      "optional_border",
      "eraser",
      "protection_zone",
      "no_pvp",
      "no_logout",
      "pvp_zone",
      "door_normal",
      "door_locked",
      "door_magic",
      "door_quest",
      "door_normal_alt",
      "window_hatch",
      "window_normal",
      "door_archway",
      "place_creature",
      "place_spawn"};

  std::filesystem::path baseDir(assetsDir);
  spdlog::info("[IconTextureCache] Loading assets from: {}", std::filesystem::absolute(baseDir).string());

  for (const auto &iconName : requiredIcons) {
    const std::string filename = iconName + ".png";
    std::filesystem::path iconPath = baseDir / filename;

    // Support fallback to relative path if executed from build directory
    if (!std::filesystem::exists(iconPath)) {
      iconPath = std::filesystem::current_path() / "assets" / "png" / filename;
    }
    if (!std::filesystem::exists(iconPath)) {
      iconPath = std::filesystem::current_path() / ".." / "assets" / "png" / filename;
    }

    if (!std::filesystem::exists(iconPath)) {
      throw std::runtime_error(std::format(
          "[IconTextureCache] Required icon asset not found: {}", filename));
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *data =
        stbi_load(iconPath.string().c_str(), &width, &height, &channels, 4);

    if (!data) {
      throw std::runtime_error(std::format(
          "[IconTextureCache] Failed to load PNG data for icon {}: {}",
          filename, stbi_failure_reason()));
    }

    // Apply background transparency filter
    makeBackgroundTransparent(data, width, height);

    auto texture = std::make_unique<Rendering::Texture>(
        static_cast<uint32_t>(width), static_cast<uint32_t>(height), data);

    stbi_image_free(data);

    if (!texture->isValid()) {
      throw std::runtime_error(std::format(
          "[IconTextureCache] Failed to create GPU texture for icon {}",
          filename));
    }

    textures_[iconName] = std::move(texture);
    spdlog::debug("[IconTextureCache] Loaded icon: {} ({}x{})", iconName, width,
                  height);
  }

  spdlog::info("[IconTextureCache] Successfully loaded all {} required icons",
               textures_.size());
}

IconTextureCache::IconInfo IconTextureCache::getIcon(const std::string &name) const {
  auto it = textures_.find(name);
  if (it == textures_.end()) {
    return IconInfo{};
  }

  const auto &tex = it->second;
  return IconInfo{
      (ImTextureID)(intptr_t)tex->id(),
      tex->getWidth(), tex->getHeight()};
}

} // namespace MapEditor::UI
