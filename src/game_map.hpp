#pragma once

#include <flecs.h>
#include <libtcod.hpp>

#include <vector>

#include "tiles.hpp"

struct GameMap : TCODMap {
  GameMap(int width, int height)
      : TCODMap(width, height), width(width), height(height),
        explored((size_t)(width * height), false),
        tiles((size_t)(width * height), tile::wall_tile) {
    clear();
  };

  bool inBounds(int x, int y) const;
  bool inBounds(std::array<int, 2> xy) const;
  tile &operator[](std::array<int, 2> xy);
  const tile &operator[](std::array<int, 2> xy) const;

  void render(tcod::Console &console) const;
  void update_fov(flecs::entity player);

private:
  int width;
  int height;
  std::vector<bool> explored;
  std::vector<tile> tiles;
};
