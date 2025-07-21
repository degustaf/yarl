#include "game_map.hpp"

#include "actor.hpp"

bool GameMap::inBounds(int x, int y) const {
  return 0 <= x && x < width && 0 <= y && y < height;
}

bool GameMap::inBounds(std::array<int, 2> xy) const {
  return inBounds(xy[0], xy[1]);
}

tile &GameMap::operator[](std::array<int, 2> xy) {
  return tiles[(size_t)(xy[1] * width + xy[0])];
}

const tile &GameMap::operator[](std::array<int, 2> xy) const {
  return tiles[(size_t)(xy[1] * width + xy[0])];
}

void GameMap::render(tcod::Console &console) const {
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < width; x++) {
      console.at(x, y) = isInFov(x, y) ? tiles[(size_t)(y * width + x)].light
                         : explored[(size_t)(y * width + x)]
                             ? tiles[(size_t)(y * width + x)].dark
                             : tile::shroud;
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
