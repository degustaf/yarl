#include "game_map.hpp"
#include "room_accretion.hpp"

#include <cstddef>

void GameMap::carveOut(int x, int y) { map.setProperties(x, y, true, true); }

GameMap GameMap::nextFloor(flecs::entity map, flecs::entity player) const {
  return generateDungeon(map, width, height, level + 1, player);
}

static constexpr auto floor_light =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {200, 180, 50, 255}};
static constexpr auto floor_dark =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {50, 50, 150, 255}};

static constexpr auto wall_light =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {130, 110, 50, 255}};
static constexpr auto wall_dark =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {0, 0, 100, 255}};

static constexpr auto stairs_light =
    TCOD_ConsoleTile{'>', {255, 255, 255, 255}, {200, 180, 50, 255}};
static constexpr auto stairs_dark =
    TCOD_ConsoleTile{'>', {255, 255, 255, 255}, {50, 50, 150, 255}};

static constexpr auto shroud =
    TCOD_ConsoleTile{' ', {255, 255, 255, 255}, {0, 0, 0, 255}};

void GameMap::render(tcod::Console &console) const {
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < width; x++) {
      if (map.isInFov(x, y)) {
        console.at(x, y) = isStairs({x, y})          ? stairs_light
                           : map.isTransparent(x, y) ? floor_light
                                                     : wall_light;
      } else if (tiles[(size_t)(y * width + x)].flags & Tile::Explored) {
        console.at(x, y) = isStairs({x, y})          ? stairs_dark
                           : map.isTransparent(x, y) ? floor_dark
                                                     : wall_dark;
      } else {
        console.at(x, y) = shroud;
      }
    }
  }
}

void GameMap::update_fov(flecs::entity player) {
  auto pos = player.get<Position>();
  map.computeFov(pos.x, pos.y, 8, true, FOV_SYMMETRIC_SHADOWCAST);
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < width; x++) {
      if (map.isInFov(x, y)) {
        tiles[(size_t)(y * width + x)].flags |= Tile::Explored;
      }
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
               .query_builder<const Position>("module::blocksPosition")
               .with(flecs::ChildOf, map)
               .with<BlocksMovement>()
               .build();
  return q.find([&](const auto &p) { return p == pos; });
}
