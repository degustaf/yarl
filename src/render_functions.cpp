#include "render_functions.hpp"

#include <array>
#include <libtcod/console_printing.hpp>
#include <optional>
#include <string>

#include "actor.hpp"
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

void renderNamesAtMouseLocation(tcod::Console &console,
                                const std::array<int, 2> xy,
                                const std::array<int, 2> &mouse_loc,
                                flecs::entity map) {
  auto q =
      map.world()
          .query_builder<const Position, const Named>("module::namedPosition")
          .with(flecs::ChildOf, map)
          .build();
  auto msg = std::string();
  q.each([&](auto &pos, auto &name) {
    if (pos == mouse_loc) {
      msg += name.name + ", ";
    }
  });
  msg = msg.substr(0, msg.size() - 2);
  tcod::print(console, xy, msg, std::nullopt, std::nullopt);
}
