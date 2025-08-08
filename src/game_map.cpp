#include "game_map.hpp"

#include "actor.hpp"
#include <cstddef>
#include <libtcod/console.h>

bool GameMap::inBounds(int x, int y) const {
  return 0 <= x && x < width && 0 <= y && y < height;
}

bool GameMap::inBounds(std::array<int, 2> xy) const {
  return inBounds(xy[0], xy[1]);
}

void GameMap::carveOut(int x, int y) {
  walls[(size_t)(y * width + x)] = false;
  setProperties(x, y, true, true);
}

static constexpr auto floor_light =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {200, 180, 50, 255}};
static constexpr auto floor_dark =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {50, 50, 150, 255}};

static constexpr auto wall_light =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {130, 110, 50, 255}};
static constexpr auto wall_dark =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {0, 0, 100, 255}};

static constexpr auto shroud =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {0, 0, 0, 255}};

void GameMap::render(tcod::Console &console) const {
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < width; x++) {
      if (isInFov(x, y)) {
        console.at(x, y) =
            walls[(size_t)(y * width + x)] ? wall_light : floor_light;
      } else if (explored[(size_t)(y * width + x)]) {
        console.at(x, y) =
            walls[(size_t)(y * width + x)] ? wall_dark : floor_dark;
      } else {
        console.at(x, y) = shroud;
      }
    }
  }
}

void GameMap::update_fov(flecs::entity player) {
  auto pos = player.get<Position>();
  computeFov(pos.x, pos.y, 8, true, FOV_SYMMETRIC_SHADOWCAST);
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < width; x++) {
      explored[(size_t)(y * width + x)] =
          explored[(size_t)(y * width + x)] || isInFov(x, y);
    }
  }
}

flecs::entity GameMap::get_blocking_entity(flecs::entity map,
                                           const Position &pos) {
  auto player = map.world().lookup("player");
  if (player.get<Position>() == pos) {
    return player;
  }
  auto q = map.world()
               .query_builder<const Position>()
               .with(flecs::ChildOf, map)
               .with<BlocksMovement>()
               .build();
  return q.find([&](const auto &p) { return p == pos; });
}
