#pragma once

#include <cstdint>
#include <vector>

#include <flecs.h>
#include <libtcod.hpp>

#include "actor.hpp"
#include "scent.hpp"

struct BlocksMovement {};
struct BlocksFov {};
struct Openable {};
struct Fountain {};
struct Portal {};
struct Light {
  int radius;
  float decayFactor;
};

struct CurrentMap {};

struct Tile {
  uint8_t flags;

  static constexpr auto Explored = uint8_t(0x1);
  static constexpr auto Stairs = uint8_t(0x2);
  static constexpr auto Bloody = uint8_t(0x4);
  static constexpr auto KnownBloody = uint8_t(0x8);
  static constexpr auto Sensed = uint8_t(0x10);
  static constexpr auto Water = uint8_t(0x20);
};

void deleteMapEntity(flecs::entity map);
void deleteMapEntity(flecs::world ecs);

struct GameMap {
  GameMap(int width = 0, int height = 0, int level = 1, bool lit = true)
      : width(width), height(height), level(level), lit(lit),
        tiles(width * height), scent(width * height),
        luminosity(width * height), map(width, height), noise(3) {
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
  inline bool inLight(std::array<int, 2> xy, float brightness = 0.0f) const {
    return luminosity[xy[1] * width + xy[0]] > brightness;
  }
  inline bool canSeePlayer(std::array<int, 2> xy,
                           std::array<int, 2> player) const {
    return map.isInFov(xy[0], xy[1]) && inLight(player);
  }
  inline bool isVisible(std::array<int, 2> xy) const {
    return isVisible(xy[0], xy[1]);
  }
  inline bool isVisible(int x, int y) const {
    return map.isInFov(x, y) && inLight({x, y});
  }
  inline void setFov(std::array<int, 2> xy, bool visible) {
    return map.setInFov(xy[0], xy[1], visible);
  }
  inline bool isTransparent(std::array<int, 2> xy) const {
    return isTransparent(xy[0], xy[1]);
  }
  inline bool isTransparent(int x, int y) const {
    return inBounds(x, y) && map.isTransparent(x, y);
  }
  inline bool isWalkable(std::array<int, 2> xy) const {
    return inBounds(xy) && isWalkable(xy[0], xy[1]);
  }
  inline bool isWalkable(int x, int y) const { return map.isWalkable(x, y); }
  inline bool isFlyable(std::array<int, 2> xy) const {
    return isFlyable(xy[0], xy[1]);
  }
  inline bool isFlyable(int x, int y) const {
    return inBounds(x, y) && isTransparent(x, y);
  }
  inline void makeStairs(std::array<int, 2> xy) {
    return makeStairs(xy[0], xy[1]);
  }
  inline void makeStairs(int x, int y) {
    tiles[(size_t)(y * width + x)].flags |= Tile::Stairs;
  }
  inline bool isStairs(std::array<int, 2> xy) const {
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
  inline bool isWater(int x, int y) const {
    return inBounds(x, y) && (tiles[y * width + x].flags & Tile::Water);
  }
  inline bool isWater(std::array<int, 2> xy) const {
    return isWater(xy[0], xy[1]);
  }
  inline bool isChasm(std::array<int, 2> xy) const {
    return isTransparent(xy) && !isWalkable(xy) && !isWater(xy);
  };
  inline Scent &getScent(std::array<int, 2> xy) {
    return scent[xy[1] * width + xy[0]];
  }
  inline const Scent &getScent(std::array<int, 2> xy) const {
    return scent[xy[1] * width + xy[0]];
  }
  inline void addLuminosity(std::array<int, 2> xy, float lumens) {
    assert(0 <= xy[0] && xy[0] < width);
    assert(0 <= xy[1] && xy[1] < height);
    auto &l = luminosity[xy[1] * width + xy[0]];
    l = std::clamp(l + lumens, 0.0f, 1.0f);
  }

  void carveOut(int x, int y);
  void nextFloor(flecs::entity player, bool lit) const;
  void render(tcod::Console &console, uint64_t time);
  void update_fov(flecs::entity mapEntity, flecs::entity player);
  void update_scent(flecs::entity map);
  void reveal();
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
  bool lit;
  std::vector<Tile> tiles;
  std::vector<Scent> scent;
  std::vector<float> luminosity;

private:
  TCODMap map;
  TCODNoise noise;
};

void computeFov(flecs::entity mapEntity, GameMap &map,
                std::array<int, 2> origin, int maxRadius);
void addLight(flecs::entity mapEntity, GameMap &map);
