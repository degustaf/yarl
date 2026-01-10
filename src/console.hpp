#pragma once

#include <libtcod.h>

struct Console : private tcod::Console {
  Console() : tcod::Console() {};
  Console(int w, int h) : tcod::Console(w, h) {};

  [[nodiscard]] const TCOD_ConsoleTile &at(const std::array<int, 2> &xy) const {
    return tcod::Console::at(xy);
  }
  [[nodiscard]] TCOD_ConsoleTile &at(const std::array<int, 2> &xy) {
    return tcod::Console::at(xy);
  }
  [[nodiscard]] inline auto get_width() const noexcept -> int {
    return tcod::Console::get_width();
  };
  [[nodiscard]] inline auto get_height() const noexcept -> int {
    return tcod::Console::get_height();
  };
  [[nodiscard]] auto inline begin() noexcept -> TCOD_ConsoleTile * {
    return tcod::Console::begin();
  }
  [[nodiscard]] auto inline begin() const noexcept -> const TCOD_ConsoleTile * {
    return tcod::Console::begin();
  }
  [[nodiscard]] auto inline end() noexcept -> TCOD_ConsoleTile * {
    return tcod::Console::end();
  }
  [[nodiscard]] auto inline end() const noexcept -> const TCOD_ConsoleTile * {
    return tcod::Console::end();
  }

  auto &userdata(void) { return get()->userdata; }
  auto &on_delete(void) { return get()->on_delete; }

  void clear(const TCOD_ConsoleTile &tile = {
                 ' ', {255, 255, 255, 255}, {0, 0, 0, 255}}) noexcept {
    for (auto &it : *this)
      it = tile;
  }

  static inline void print(Console &console, const std::array<int, 2> &xy,
                           std::string_view str,
                           std::optional<TCOD_ColorRGB> fg,
                           std::optional<TCOD_ColorRGB> bg,
                           TCOD_alignment_t alignment = TCOD_LEFT,
                           TCOD_bkgnd_flag_t flag = TCOD_BKGND_SET) {
    return tcod::print(console, xy, str, fg, bg, alignment, flag);
  };

  static inline int print_rect(Console &console, const std::array<int, 4> &rect,
                               std::string_view str,
                               std::optional<TCOD_ColorRGB> fg,
                               std::optional<TCOD_ColorRGB> bg,
                               TCOD_alignment_t alignment = TCOD_LEFT,
                               TCOD_bkgnd_flag_t flag = TCOD_BKGND_SET) {
    return tcod::print_rect(console, rect, str, fg, bg, alignment, flag);
  };

  static inline void draw_rect(Console &console, const std::array<int, 4> &rect,
                               int ch, std::optional<TCOD_ColorRGB> fg,
                               std::optional<TCOD_ColorRGB> bg,
                               TCOD_bkgnd_flag_t flag = TCOD_BKGND_SET) {
    tcod::draw_rect(console, rect, ch, fg, bg, flag);
  }

  static inline void
  draw_frame(Console &console, const std::array<int, 4> &rect,
             const std::array<int, 9> &decoration,
             std::optional<TCOD_ColorRGB> fg, std::optional<TCOD_ColorRGB> bg,
             TCOD_bkgnd_flag_t flag = TCOD_BKGND_SET, bool clear = true) {
    tcod::draw_frame(console, rect, decoration, fg, bg, flag, clear);
  }

  static inline void blit(Console &dest, const Console &source,
                          const std::array<int, 2> &dest_xy = {0, 0},
                          std::array<int, 4> source_rect = {0, 0, 0, 0},
                          float foreground_alpha = 1.0f,
                          float background_alpha = 1.0f) {
    tcod::blit(dest, source, dest_xy, source_rect, foreground_alpha,
               background_alpha);
  };
};
