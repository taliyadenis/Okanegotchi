#pragma once
#include "protocol.hpp"
#include <cstddef>
#include <cstdint>

namespace okanegachi::assets {
struct Point {
  int x, y;
};
struct Frame {
  const char *id;
  int width, height;
  int indexed; // 0 RGB565, 1 packed 4-bit, 2 byte indices
  const uint8_t *bytes;
  const uint8_t *mask;
  Point feet, head, neck, body;
  const uint16_t *fixed_palette{
      nullptr}; // Authored pet colors take precedence.
};
struct Step {
  const Frame *frame;
  uint32_t duration;
};
struct Clip {
  const char *id;
  const Step *steps;
  size_t count;
  bool loop;
};
struct Palette {
  const char *id;
  uint16_t colors[16];
};
struct Character {
  const char *id;
  const Clip *clips[11];
};
struct Accessory {
  const char *id;
  const char *character;
  const Frame *frame;
  int anchor;
  Point origin, offset;
  bool front;
};
struct Prop {
  const Frame *frame;
  Point xy;
  int scale;
  bool front;
};
struct Scene {
  const char *id;
  uint16_t color;
  const Frame *background;
  Point root;
  int scale;
  const Prop *props;
  size_t count;
};
extern const Palette palettes[];
extern const size_t palette_count;
extern const Character characters[];
extern const size_t character_count;
extern const Accessory accessories[];
extern const size_t accessory_count;
extern const Scene *bindings[11];
extern const size_t pixel_bytes;
const Frame &frame_at(const Clip &, uint64_t elapsed);
// Fill an arbitrary strip of a 240 x 240 scene. No allocation or display
// dependency. The caller owns the tile and transmits it before reusing it.
void render(uint16_t *pixels, int y, int height, const Appearance &,
            const std::string &animation, uint64_t elapsed);
} // namespace okanegachi::assets
