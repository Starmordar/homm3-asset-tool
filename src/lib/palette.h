#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>

namespace Palette {
  struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
  };

  inline const std::unordered_map<int, Color> predefined_palette_indices{
      {0, {0, 0, 0, 0}},   // Transparency                 (0,255,255)
      {1, {0, 0, 0, 64}},  // Shadow border                (255,150,255)
      {4, {0, 0, 0, 128}}, // Shadow body                  (255,0,255)
      {5, {0, 0, 0, 0}},   // Selection                    (255,255,0)
      {6, {0, 0, 0, 128}}, // Selection over shadow body   (180,0,255)
      {7, {0, 0, 0, 64}},  // Selection over shadow border (0,255,0)
  };
} // namespace Palette
