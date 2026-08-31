#include "render_functions.hpp"

#include <SDL3/SDL.h>

#include <optional>
#include <string>

#include "actor.hpp"
#include "color.hpp"
#include "defines.hpp"
#include "map_queries.hpp"
#include "scent.hpp"
#include "string.hpp"

void renderBar(Console &console, int currentValue, int maxValue, int x, int y,
               int totalWidth) {
  auto bar_width = (int)(((double)currentValue) / maxValue * totalWidth);

  console.draw_rect({x, y, totalWidth, 1}, ' ', std::nullopt, Colors::barEmpty);

  if (bar_width > 0) {
    console.draw_rect({x, y, bar_width, 1}, ' ', std::nullopt,
                      Colors::barFilled);
  }

  auto msg = stringf("HP: %d/%d", currentValue, maxValue);
  console.print({x + 1, y}, msg, Colors::barText, std::nullopt);
}

void renderSmell(Console &console, flecs::entity player, int x, int y,
                 int totalWidth) {
  auto scent = player.get<Scent>();
  auto bg = [](auto scent) -> color::RGBA {
    switch (scent.type) {
    case ScentType::player:
      return scent.power < 25.0f   ? Colors::go
             : scent.power < 50.0f ? Colors::caution
             : scent.power < 75.0f ? Colors::extraCaution
                                   : Colors::stop;
    case ScentType::fiend:
    case ScentType::decay:
      return Colors::dung;
    case ScentType::none:
    case ScentType::MAX:
      break;
    }
    assert(false);
    return Colors::text;
  }(scent);
  auto bar_width =
      std::min((int)((scent.power * (float)totalWidth) / 100.0f), totalWidth);

  console.draw_rect({x, y, totalWidth, 1}, ' ', std::nullopt, Colors::barEmpty);
  if (bar_width > 0) {
    console.draw_rect({x, y, bar_width, 1}, ' ', std::nullopt, bg);
  }

  auto msg = stringf("Scent: %d", (int)scent.power);
  console.print({x + 1, y}, msg, Colors::barText, std::nullopt);
  if (bg == Colors::caution) {
    for (x++; x < bar_width; x++) {
      console.at({x, y}).fg = Colors::background;
    }
  }
}

void renderDungeonLevel(Console &console, int level,
                        std::array<int, 2> location) {
  auto msg = stringf("Dungeon level: %d", level);
  console.print(location, msg, std::nullopt, std::nullopt);
}

void renderNamesAtMouseLocation(Console &console, const std::array<int, 2> &xy,
                                const std::array<int, 2> &mouse_loc,
                                flecs::entity map, const GameMap &gameMap) {
  if (!gameMap.isExplored(mouse_loc))
    return;

  auto q =
      mapQuery<positionQuery::NamedStackable, const Named, const Stackable *>(
          map.world(), map);
  auto msg = std::string();
  q.each([&](auto &pos, auto &name, auto *s) {
    if (pos == mouse_loc) {
      if (s && s->count > 1) {
        msg += std::to_string(s->count) + " " + name.name + ", ";
      } else {
        msg += name.name + ", ";
      }
    }
  });
  if (msg.size() > 0) {
    msg = msg.substr(0, msg.size() - 2);
  }
  if (gameMap.isKnownBloody(mouse_loc)) {
    msg = msg + (msg.size() > 0 ? ", " : "") + "a bloody trail";
  }
  if (gameMap.isStairsDown(mouse_loc)) {
    msg = msg + (msg.size() > 0 ? ", " : "") + "stairs down";
  }
  if (gameMap.isStairsUp(mouse_loc)) {
    msg = msg + (msg.size() > 0 ? ", " : "") + "stairs up";
  }
  console.print(xy, msg, std::nullopt, std::nullopt);
}

void renderDescribableAtMouseLocation(Console &console,
                                      const std::array<int, 2> &mouse_loc,
                                      flecs::entity e) {
  assert(e.has<Describable>());
  static constexpr auto width = 15;
  static constexpr auto height = 5;
  auto x = mouse_loc[0] + 1 + width > console.get_width()
               ? mouse_loc[0] - 1 - width
               : mouse_loc[0] + 1;
  auto y = mouse_loc[1] + height > console.get_height() ? mouse_loc[1]
                                                        : mouse_loc[1] - height;
  console.draw_frame({x, y, width, height}, DECORATION, Colors::text,
                     Colors::background);
  console.print({x + 1, y + 1}, Describable::describe(e), Colors::text,
                std::nullopt);
}

void renderCommandButton(Console &console, const std::array<int, 4> &xywh) {
  console.draw_frame(xywh, DECORATION, Colors::menu_border,
                     Colors::menu_background);
  auto key = *SDL_GetKeyName(
      SDL_GetKeyFromScancode(SDL_SCANCODE_C, SDL_KMOD_NONE, true));
  auto str = key == 'C' ? "(C)ommands" : stringf("(%c)Commands", key);
  assert(str.size() + 2 <= (size_t)xywh[2]);
  console.print({xywh[0] + 1, xywh[1] + 1}, str, std::nullopt, std::nullopt);
}
