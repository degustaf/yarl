#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

#include <flecs.h>

#include "random.hpp"
#include "scent.hpp"

struct BlocksMovement {};
struct BlocksFov {};
struct Openable {};
struct Fountain {};
struct Portal {};
struct Light {
  int innerRadius;
  int outerRadius;
  float decayFactor;
};

struct PrevMap {};
struct CurrentMap {};
struct NextMap {};

struct Tile {
  using type = uint16_t;
  Tile() : flags(0), scent(), luminosity(0.0f) {};

  type flags;
  Scent scent;
  float luminosity;

  static constexpr auto Walkable = type(0x01);
  static constexpr auto Transparent = type(0x02);
  static constexpr auto inFov = type(0x04);
  static constexpr auto Explored = type(0x8);
  static constexpr auto StairsDown = type(0x10);
  static constexpr auto StairsUp = type(0x20);
  static constexpr auto Bloody = type(0x40);
  static constexpr auto KnownBloody = type(0x80);
  static constexpr auto Sensed = type(0x100);
  static constexpr auto Water = type(0x200);
};

void deleteMapQueries(flecs::world ecs);
void deleteMapEntity(flecs::world ecs, flecs::entity map);
void deleteMapEntity(flecs::entity map);
void deleteMapEntity(flecs::world ecs);

struct GameMap {
  GameMap(int width = 0, int height = 0, int level = 1, bool lit = true)
      : width(width), height(height), level(level), lit(lit),
        tiles(width * height, Tile()), noise() {};

  inline int getWidth() const { return width; }
  inline int getHeight() const { return height; }
  inline bool inBounds(int x, int y) const {
    return 0 <= x && x < getWidth() && 0 <= y && y < getHeight();
  }
  inline bool inBounds(std::array<int, 2> xy) const {
    return inBounds(xy[0], xy[1]);
  }
  inline bool inLight(std::array<int, 2> xy, float brightness = 0.0f) const {
    return tiles[xy[1] * width + xy[0]].luminosity > brightness;
  }
  inline bool canSeePlayer(std::array<int, 2> xy,
                           std::array<int, 2> player) const {
    return isInFov(xy) && inLight(player);
  }
  inline bool isVisible(std::array<int, 2> xy) const {
    return isVisible(xy[0], xy[1]);
  }
  inline bool isVisible(int x, int y) const {
    return inBounds(x, y) && isInFov(std::array<int, 2>{x, y}) &&
           inLight({x, y});
  }
  inline bool isInFov(std::array<int, 2> xy) const {
    return tile(xy).flags & Tile::inFov;
  }
  inline bool isInFov(std::array<float, 2> xy) const {
    return isInFov(std::array<int, 2>{(int)xy[0], (int)xy[1]});
  }
  inline void setFov(std::array<int, 2> xy, bool visible) {
    if (visible) {
      tile(xy).flags |= Tile::inFov;
    } else {
      tile(xy).flags &= ~Tile::inFov;
    }
  }
  inline bool isTransparent(std::array<int, 2> xy) const {
    return inBounds(xy) && (tile(xy).flags & Tile::Transparent);
  }
  inline bool isTransparent(int x, int y) const {
    return isTransparent(std::array<int, 2>{x, y});
  }
  inline bool isWalkable(std::array<int, 2> xy) const {
    return inBounds(xy) && (tile(xy).flags & Tile::Walkable);
  }
  inline bool isWalkable(int x, int y) const {
    return isWalkable(std::array<int, 2>{x, y});
  }
  inline bool isFlyable(std::array<int, 2> xy) const {
    return isFlyable(xy[0], xy[1]);
  }
  inline bool isFlyable(int x, int y) const {
    return inBounds(x, y) && isTransparent(x, y);
  }
  inline void makeStairs(std::array<int, 2> xy, bool down) {
    return makeStairs(xy[0], xy[1], down);
  }
  inline void makeStairs(int x, int y, bool down) {
    if (down) {
      tiles[(size_t)(y * width + x)].flags |= Tile::StairsDown;
    } else {
      tiles[(size_t)(y * width + x)].flags |= Tile::StairsUp;
    }
  }
  inline bool isStairsDown(std::array<int, 2> xy) const {
    return tiles[(size_t)(xy[1] * width + xy[0])].flags & Tile::StairsDown;
  };
  inline bool isStairsUp(std::array<int, 2> xy) const {
    return tiles[(size_t)(xy[1] * width + xy[0])].flags & Tile::StairsUp;
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
    return tiles[xy[1] * width + xy[0]].scent;
  }
  inline const Scent &getScent(std::array<int, 2> xy) const {
    return tiles[xy[1] * width + xy[0]].scent;
  }
  inline void addLuminosity(std::array<int, 2> xy, float lumens) {
    assert(0 <= xy[0] && xy[0] < width);
    assert(0 <= xy[1] && xy[1] < height);
    auto &l = tiles[xy[1] * width + xy[0]].luminosity;
    l = std::clamp(l + lumens, 0.0f, 1.0f);
  }
  inline Tile &tile(std::array<int, 2> xy) {
    return tiles[xy[1] * width + xy[0]];
  }
  inline const Tile &tile(std::array<int, 2> xy) const {
    return tiles[xy[1] * width + xy[0]];
  }

  void carveOut(int x, int y);
  void nextFloor(flecs::entity player, bool lit) const;
  void render(Console &console, uint64_t);
  void update_fov(flecs::entity mapEntity, flecs::entity player);
  void update_scent(flecs::entity map);
  void reveal();
  inline void setProperties(int x, int y, bool isTransparent, bool isWalkable) {
    assert(inBounds(x, y));
    auto &flags = tile(std::array<int, 2>{x, y}).flags;
    if (isTransparent) {
      flags |= Tile::Transparent;
    } else {
      flags &= ~Tile::Transparent;
    }
    if (isWalkable) {
      flags |= Tile::Walkable;
    } else {
      flags &= ~Tile::Walkable;
    }
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

private:
  Noise<3> noise;
};
