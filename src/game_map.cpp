#include "game_map.hpp"

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
      console.at(x, y) = tiles[(size_t)(y * width + x)].dark;
    }
  }
}
