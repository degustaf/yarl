#pragma once

#include "color.hpp"

#include <array>
#include <cassert>
#include <memory>
#include <optional>
#include <string_view>

struct Console {

  enum class Alignment { LEFT, RIGHT, CENTER };

  struct Tile {
    int ch;
    color::RGBA fg;
    color::RGBA bg;
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
  [[nodiscard]] inline auto get_width() const noexcept -> int { return w; };
  [[nodiscard]] inline auto get_height() const noexcept -> int { return h; };
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
};
