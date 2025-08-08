#include "render_functions.hpp"

#include <optional>

#include "color.hpp"

void renderBar(tcod::Console &console, int currentValue, int maxValue,
               int totalWidth) {
  auto bar_width = (int)(((double)currentValue) / maxValue * totalWidth);

  tcod::draw_rect(console, {0, 45, totalWidth, 1}, 1, std::nullopt,
                  color::barEmpty);

  if (bar_width > 0) {
    tcod::draw_rect(console, {0, 45, bar_width, 1}, 1, std::nullopt,
                    color::barFilled);
  }

  auto msg = tcod::stringf("HP: %d/%d", currentValue, maxValue);
  tcod::print(console, {1, 45}, msg, color::barText, std::nullopt);
}
