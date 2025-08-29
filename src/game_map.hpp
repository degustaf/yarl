#pragma once

#include <flecs.h>
#include <libtcod.hpp>

#include <vector>

#include "actor.hpp"

struct CurrentMap {};

struct Tile {
  uint8_t flags;

  static constexpr auto Explored = uint8_t(1);
};

struct GameMap {
  GameMap(int width = 0, int height = 0, uint32_t seed = 0)
      : width(width), height(height), seed(seed), tiles(width * height),
        map(width, height) {
    map.clear();
  };

  void init() { map = TCODMap(width, height); }

  bool inBounds(int x, int y) const;
  bool inBounds(std::array<int, 2> xy) const;
  inline int getWidth() const { return width; }
  inline int getHeight() const { return height; }
  bool isInFov(std::array<int, 2> xy) const {
    return map.isInFov(xy[0], xy[1]);
  }

  bool isWalkable(std::array<int, 2> xy) const {
    return map.isWalkable(xy[0], xy[1]);
  }
  void carveOut(int x, int y);

  void render(tcod::Console &console) const;
  void update_fov(flecs::entity player);

  static flecs::entity get_blocking_entity(flecs::entity map,
                                           const Position &pos);

  int width;
  int height;
  uint32_t seed;
  std::vector<Tile> tiles;

private:
  TCODMap map;
};
