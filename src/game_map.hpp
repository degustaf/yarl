#pragma once

#include <iostream>
#include <vector>

#include <flecs.h>
#include <libtcod.hpp>

#include "actor.hpp"
#include "scent.hpp"

struct CurrentMap {};

struct Tile {
  Tile() : flags(0) {};
  uint8_t flags = 0;

  static constexpr auto Explored = uint8_t(0x1);
  static constexpr auto Stairs = uint8_t(0x2);
  static constexpr auto Bloody = uint8_t(0x4);
  static constexpr auto KnownBloody = uint8_t(0x8);
  static constexpr auto Sensed = uint8_t(0x10);
};

void deleteMapEntity(flecs::entity map);
void deleteMapEntity(flecs::world ecs);

struct GameMap {
  GameMap(int width = 0, int height = 0, int level = 1)
      : width(width), height(height), level(level),
        tiles(width * height, Tile()), scent(width * height, Scent()),
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
  inline bool isTransparent(std::array<int, 2> xy) const {
    return isTransparent(xy[0], xy[1]);
  }
  inline bool isTransparent(int x, int y) const {
    return map.isTransparent(x, y);
  }
  inline bool isWalkable(std::array<int, 2> xy) const {
    return isWalkable(xy[0], xy[1]);
  }
  inline bool isWalkable(int x, int y) const { return map.isWalkable(x, y); }
  inline void makeStairs(std::array<int, 2> xy) {
    return makeStairs(xy[0], xy[1]);
  }
  inline void makeStairs(int x, int y) {
    tiles[(size_t)(y * width + x)].flags |= Tile::Stairs;
  }
  inline bool isStairs(std::array<int, 2> xy) const {
    std::cout << xy[0] << "\n";
    std::cout << xy[1] << "\n";
    std::cout << width << "\n";
    std::cout << tiles[(size_t)(xy[1] * width + xy[0])].flags << "\n";
    return tiles[(size_t)(xy[1] * width + xy[0])].flags & Tile::Stairs;
  };
  inline void makeBloody(std::array<int, 2> xy) {
    tiles[(size_t)(xy[1] * width + xy[0])].flags |= Tile::Bloody;
  };
  inline bool isBloody(std::array<int, 2> xy) const {
    return tiles[(size_t)(xy[1] * width + xy[0])].flags & Tile::Bloody;
  };
  inline bool isKnownBloody(std::array<int, 2> xy) const {
    return tiles[(size_t)(xy[1] * width + xy[0])].flags & Tile::KnownBloody;
  };
  inline bool isExplored(int x, int y) const {
    return inBounds(x, y) && (tiles[y * width + x].flags & Tile::Explored);
  };
  inline bool isExplored(std::array<int, 2> xy) const {
    return isExplored(xy[0], xy[1]);
  }
  inline bool isSensed(int x, int y) const {
    return inBounds(x, y) && (tiles[y * width + x].flags & Tile::Sensed);
  };
  inline bool isSensed(std::array<int, 2> xy) const {
    return isSensed(xy[0], xy[1]);
  }
  inline bool isChasm(std::array<int, 2> xy) const {
    return isTransparent(xy) && !isWalkable(xy);
  };
  inline Scent &getScent(std::array<int, 2> xy) {
    return scent[xy[1] * width + xy[0]];
  }
  inline const Scent &getScent(std::array<int, 2> xy) const {
    return scent[xy[1] * width + xy[0]];
  }

  void carveOut(int x, int y);
  void nextFloor(flecs::entity player) const;
  void render(Console &console) const;
  void update_fov(flecs::entity player);
  void update_scent(flecs::entity map);
  void reveal();
  inline TCODPath path(void) const { return TCODPath(&map); };
  inline const TCODMap &get(void) const { return map; };
  inline void setProperties(int x, int y, bool isTransparent, bool isWalkable) {
    map.setProperties(x, y, isTransparent, isWalkable);
  }
  ScentType detectScent(flecs::entity e, std::array<int, 2> &strongest) const;
  std::string detectScent(flecs::entity e) const;

  static flecs::entity get_blocking_entity(flecs::entity map,
                                           const Position &pos);

  int width;
  int height;
  int level;
  std::vector<Tile> tiles;
  std::vector<Scent> scent;

private:
  TCODMap map;
};
