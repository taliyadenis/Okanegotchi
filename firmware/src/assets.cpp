#include "assets.hpp"
#include <algorithm>
#include <cstring>

namespace okanegachi::assets {
const Frame &frame_at(const Clip &clip, uint64_t elapsed) {
  uint32_t total = 0;
  for (size_t i = 0; i < clip.count; ++i)
    total += clip.steps[i].duration;
  if (clip.loop)
    elapsed %= total;
  for (size_t i = 0; i < clip.count; ++i) {
    if (elapsed < clip.steps[i].duration)
      return *clip.steps[i].frame;
    elapsed -= clip.steps[i].duration;
  }
  return *clip.steps[clip.count - 1].frame;
}
namespace {
const char *states[] = {"idle",      "blink",   "pat",    "needs_checkin",
                        "sleepy",    "ghost",   "eating", "traveling",
                        "celebrate", "neutral", "revive"};
void blit(uint16_t *out, int top, int height, const Frame &f, Point at,
          int scale, const Palette &pal) {
  for (int y = std::max(top, at.y);
       y < std::min(top + height, at.y + f.height * scale); ++y)
    for (int x = std::max(0, at.x); x < std::min(240, at.x + f.width * scale);
         ++x) {
      size_t n = size_t((y - at.y) / scale) * f.width + (x - at.x) / scale;
      uint16_t color;
      if (f.indexed) {
        uint8_t index = f.indexed == 2
                            ? f.bytes[n]
                            : (f.bytes[n / 2] >> (n % 2 ? 0 : 4)) & 15;
        if (!index)
          continue;
        color = f.fixed_palette ? f.fixed_palette[index] : pal.colors[index];
      } else {
        if (f.mask && !(f.mask[n / 8] & (0x80 >> (n % 8))))
          continue;
        color = uint16_t(f.bytes[2 * n]) | (uint16_t(f.bytes[2 * n + 1]) << 8);
      }
      out[(y - top) * 240 + x] = color;
    }
}
} // namespace
void render(uint16_t *out, int y, int height, const Appearance &a,
            const std::string &animation, uint64_t elapsed) {
  if (!out || y < 0 || height < 1 || y + height > 240)
    return;
  int semantic = 0;
  for (int i = 0; i < 11; ++i)
    if (animation == states[i])
      semantic = i;
  const Character *character = &characters[0];
  for (size_t i = 0; i < character_count; ++i)
    if (a.pet == characters[i].id)
      character = &characters[i];
  const Palette *palette = &palettes[0];
  for (size_t i = 0; i < palette_count; ++i)
    if (a.palette == palettes[i].id)
      palette = &palettes[i];
  const auto &frame = frame_at(*character->clips[semantic], elapsed);
  const auto &scene = *bindings[semantic];
  std::fill(out, out + 240 * height, scene.color);
  if (scene.background)
    blit(out, y, height, *scene.background, {0, 0},
         240 / scene.background->width, *palette);
  Point pos{scene.root.x - frame.feet.x * scene.scale,
            scene.root.y - frame.feet.y * scene.scale};
  const Accessory *clothing = nullptr;
  for (size_t i = 0; i < accessory_count; ++i)
    if (a.accessory == accessories[i].id && a.pet == accessories[i].character)
      clothing = &accessories[i];
  auto dress = [&]() {
    if (!clothing)
      return;
    Point anchors[] = {frame.feet, frame.head, frame.neck, frame.body};
    auto anchor = anchors[clothing->anchor];
    Point xy{pos.x + (anchor.x - clothing->origin.x + clothing->offset.x) *
                         scene.scale,
             pos.y + (anchor.y - clothing->origin.y + clothing->offset.y) *
                         scene.scale};
    blit(out, y, height, *clothing->frame, xy, scene.scale, *palette);
  };
  for (size_t i = 0; i < scene.count; ++i)
    if (!scene.props[i].front)
      blit(out, y, height, *scene.props[i].frame, scene.props[i].xy,
           scene.props[i].scale, *palette);
  if (clothing && !clothing->front)
    dress();
  blit(out, y, height, frame, pos, scene.scale, *palette);
  if (clothing && clothing->front)
    dress();
  for (size_t i = 0; i < scene.count; ++i)
    if (scene.props[i].front)
      blit(out, y, height, *scene.props[i].frame, scene.props[i].xy,
           scene.props[i].scale, *palette);
}
} // namespace okanegachi::assets
