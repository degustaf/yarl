#pragma once

#include <flecs.h>
#include <libtcod.hpp>

#include <vector>

struct CurrentMap {};

struct GameMap : TCODMap {
  GameMap(int width, int height)
      : TCODMap(width, height), width(width), height(height),
        explored((size_t)(width * height), false),
        walls((size_t)(width * height), true) {
    clear();
  };

  bool inBounds(int x, int y) const;
  bool inBounds(std::array<int, 2> xy) const;

  bool isWalkable(std::array<int, 2> xy) const {
    return TCODMap::isWalkable(xy[0], xy[1]);
  }
  void carveOut(int x, int y);

  void render(tcod::Console &console) const;
  void update_fov(flecs::entity player);

private:
  int width;
  int height;
  std::vector<bool> explored;
  std::vector<bool> walls;
};
