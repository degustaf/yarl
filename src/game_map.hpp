#pragma once

#include <vector>

#include "tiles.hpp"

struct GameMap {
  GameMap(int width, int height)
      : width(width), height(height),
        tiles((size_t)(width * height), tile::floor_tile) {
    tiles[20 * (size_t)width + 30] = tile::wall_tile;
    tiles[20 * (size_t)width + 31] = tile::wall_tile;
    tiles[20 * (size_t)width + 32] = tile::wall_tile;
  };

  bool inBounds(int x, int y) const;
  bool inBounds(std::array<int, 2> xy) const;
  tile &operator[](std::array<int, 2> xy);
  const tile &operator[](std::array<int, 2> xy) const;

  void render(tcod::Console &console) const;

private:
  int width;
  int height;
  std::vector<tile> tiles;
};
