#pragma once

#include <flecs.h>
#include <libtcod.hpp>

#include <vector>

#include "actor.hpp"

struct CurrentMap {};

struct Tile {
  uint8_t flags;

  static constexpr auto Explored = uint8_t(0x1);
  static constexpr auto Stairs = uint8_t(0x2);
};

struct GameMap {
  GameMap(int width = 0, int height = 0, int level = 1)
      : width(width), height(height), level(level), tiles(width * height),
        map(width, height) {
    map.clear();
  };

  void init() { map = TCODMap(width, height); }

  inline bool inBounds(int x, int y) const {
    return 0 <= x && x < width && 0 <= y && y < height;
  }
  inline bool inBounds(std::array<int, 2> xy) const {
    return inBounds(xy[0], xy[1]);
  }
  inline int getWidth() const { return width; }
  inline int getHeight() const { return height; }
  inline bool isInFov(std::array<int, 2> xy) const {
    return map.isInFov(xy[0], xy[1]);
  }
  inline bool isWalkable(std::array<int, 2> xy) const {
    return map.isWalkable(xy[0], xy[1]);
  }
  inline bool isStairs(std::array<int, 2> xy) const {
    return tiles[(size_t)(xy[1] * width + xy[0])].flags & Tile::Stairs;
  }

  void carveOut(int x, int y);
  GameMap nextFloor(flecs::entity map, flecs::entity player) const;
  void render(tcod::Console &console) const;
  void update_fov(flecs::entity player);

  static flecs::entity get_blocking_entity(flecs::entity map,
                                           const Position &pos);

  int width;
  int height;
  int level;
  std::vector<Tile> tiles;

private:
  TCODMap map;
};
