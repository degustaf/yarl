#pragma once

#include "color.hpp"

#include <array>
#include <cassert>
#include <cstring>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include <utf8proc.h>

struct Console {

  enum class Alignment { LEFT, RIGHT, CENTER };

  struct Tile {
    inline void encodeChar(int c) { ch = c; };

    inline void copyChar(const Tile &other) { ch = other.ch; };

    inline bool sameChar(const Tile &other) { return ch == other.ch; };

    Console::Tile normalize_tile_for_drawing() const {
      auto tile = *this;
      if (tile.ch < 0) {
        tile.encodeChar(0); // Tile character is out-of-bounds.
      }
      if (tile.fg.a == 0)
        tile.encodeChar(0); // No foreground alpha.
      if (tile.bg.r == tile.fg.r && tile.bg.g == tile.fg.g &&
          tile.bg.b == tile.fg.b && tile.bg.a == 255 && tile.fg.a == 255) {
        tile.encodeChar(0); // Foreground and background color match, so the
                            // foreground glyph would be invisible.
      }
      if (tile.ch == 0) {
        tile.fg.r = tile.fg.g = tile.fg.b = tile.fg.a = 0;
        // Clear foreground color if the foreground glyph is skipped.
      }
      return tile;
    }

    inline constexpr bool operator==(const Tile &rhs) const {
      return fg == rhs.fg && bg == rhs.bg && ch == rhs.ch;
    }

    int ch;
    color::RGBA fg;
    color::RGBA bg;
  };

  struct offGrid {
    int ch;
    color::RGBA fg;
    float x;
    float y;
    float scale; // 1.0f means use the default font size.
  };

  Console() = default;
  Console(int w, int h)
      : w(w), h(h), elements(w * h), fg(color::white), bg(color::black),
        key_color(std::nullopt),
        tiles(std::make_unique<Tile[]>((size_t)elements)) {
    clear({' ', fg, bg});
  };

  [[nodiscard]] inline const Tile &at(const std::array<int, 2> &xy) const {
    assert(0 <= xy[0] && xy[0] <= w);
    assert(0 <= xy[1] && xy[1] <= h);
    return tiles[(size_t)(w * xy[1] + xy[0])];
  }
  [[nodiscard]] inline Tile &at(const std::array<int, 2> &xy) {
    assert(0 <= xy[0] && xy[0] <= w);
    assert(0 <= xy[1] && xy[1] <= h);
    return tiles[(size_t)(w * xy[1] + xy[0])];
  }
  [[nodiscard]] inline int get_width() const noexcept { return w; };
  [[nodiscard]] inline int get_height() const noexcept { return h; };
  [[nodiscard]] inline std::array<int, 2> get_dims() const noexcept {
    return {w, h};
  };
  [[nodiscard]] inline Tile *begin() noexcept { return tiles.get(); }
  [[nodiscard]] inline const Tile *begin() const noexcept {
    return tiles.get();
  }
  [[nodiscard]] inline Tile *end() noexcept { return tiles.get() + elements; }
  [[nodiscard]] inline const Tile *end() const noexcept {
    return tiles.get() + elements;
  }

  void clear(const Tile &tile = {' ', color::white, color::black}) noexcept {
    for (auto &it : *this)
      it = tile;
    chars.clear();
  }

  void addOffGrid(int ch, color::RGBA fg, std::array<float, 2> pos,
                  float scale = 1.0f) {
    chars.push_back({ch, fg, pos[0], pos[1], scale});
  }

  void print(const std::array<int, 2> &xy, std::string_view str,
             std::optional<color::RGBA> fg, std::optional<color::RGBA> bg,
             Alignment alignment = Alignment::LEFT);

  int print_rect(const std::array<int, 4> &rect, std::string_view str);

  void draw_rect(const std::array<int, 4> &rect, int ch,
                 std::optional<color::RGBA> fg, std::optional<color::RGBA> bg);

  void draw_frame(const std::array<int, 4> &rect,
                  const std::array<int, 9> &decoration,
                  std::optional<color::RGBA> fg, std::optional<color::RGBA> bg);

  void blit(const Console &source, const std::array<int, 2> &dest_xy = {0, 0},
            std::array<int, 4> source_rect = {0, 0, 0, 0},
            float foreground_alpha = 1.0f, float background_alpha = 1.0f);

  void put_rgba(int x, int y, int ch, std::optional<color::RGBA> fg,
                std::optional<color::RGBA> bg);

private:
  int w;
  int h;
  int elements;
  color::RGBA fg;
  color::RGBA bg;
  std::optional<color::RGBA> key_color;
  std::unique_ptr<Tile[]> tiles;

public:
  std::vector<offGrid> chars;
};
