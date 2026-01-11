#include "render_functions.hpp"

#include <optional>
#include <string>

#include "actor.hpp"
#include "color.hpp"
#include "defines.hpp"
#include "scent.hpp"

void renderBar(Console &console, int currentValue, int maxValue, int x, int y,
               int totalWidth) {
  auto bar_width = (int)(((double)currentValue) / maxValue * totalWidth);

  console.draw_rect({x, y, totalWidth, 1}, ' ', std::nullopt, color::barEmpty);

  if (bar_width > 0) {
    console.draw_rect({x, y, bar_width, 1}, ' ', std::nullopt,
                      color::barFilled);
  }

  auto msg = tcod::stringf("HP: %d/%d", currentValue, maxValue);
  console.print({x + 1, y}, msg, color::barText, std::nullopt);
}

void renderSmell(Console &console, flecs::entity player, int x, int y,
                 int totalWidth) {
  auto scent = player.get<Scent>();
  auto bg = [](auto scent) {
    switch (scent.type) {
    case ScentType::player:
      return scent.power < 25.0f   ? color::green
             : scent.power < 50.0f ? color::yellow
             : scent.power < 75.0f ? color::orange
                                   : color::red;
    case ScentType::fiend:
    case ScentType::decay:
      return color::brown;
    case ScentType::none:
    case ScentType::MAX:
      break;
    }
    assert(false);
    return color::white;
  }(scent);
  auto bar_width =
      std::min((int)((scent.power * (float)totalWidth) / 100.0f), totalWidth);

  console.draw_rect({x, y, totalWidth, 1}, ' ', std::nullopt, color::barEmpty);
  if (bar_width > 0) {
    console.draw_rect({x, y, bar_width, 1}, ' ', std::nullopt, bg);
  }

  auto msg = tcod::stringf("Scent: %d", (int)scent.power);
  console.print({x + 1, y}, msg, color::barText, std::nullopt);
  if (bg == color::yellow) {
    for (x++; x < bar_width; x++) {
      console.at({x, y}).fg = color::black;
    }
  }
}

void renderDungeonLevel(Console &console, int level,
                        std::array<int, 2> location) {
  auto msg = tcod::stringf("'\U0001F389'Dungeon level: %d", level);
  console.print(location, msg, std::nullopt, std::nullopt);
}

void renderNamesAtMouseLocation(Console &console, const std::array<int, 2> &xy,
                                const std::array<int, 2> &mouse_loc,
                                flecs::entity map, const GameMap &gameMap) {
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
  if (msg.size() > 0) {
    msg = msg.substr(0, msg.size() - 2);
  }
  if (gameMap.isKnownBloody(mouse_loc)) {
    msg = msg + (msg.size() > 0 ? ", " : "") + "a bloody trail";
  }
  if (gameMap.isStairs(mouse_loc)) {
    msg = msg + (msg.size() > 0 ? ", " : "") + "an elevator";
  }
  console.print(xy, msg, std::nullopt, std::nullopt);
}

void renderCommandButton(Console &console, const std::array<int, 4> &xywh) {
  console.draw_frame(xywh, DECORATION, color::menu_border, color::darkGrey);
  console.print({xywh[0] + 1, xywh[1] + 1}, "(C)ommands", std::nullopt,
                std::nullopt);
}
