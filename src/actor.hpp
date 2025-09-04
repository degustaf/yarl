#pragma once

#include <flecs.h>
#include <libtcod.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct Position {
  Position() : x(0), y(0){};
  Position(int x, int y) : x(x), y(y){};
  Position(std::array<int, 2> xy) : x(xy[0]), y(xy[1]){};
  operator std::array<int, 2>() const { return {x, y}; };
  operator std::array<size_t, 2>() const { return {(size_t)x, (size_t)y}; };
  Position operator+(std::array<int, 2> dxy) const {
    return {x + dxy[0], y + dxy[1]};
  };
  bool operator==(const Position &rhs) const {
    return x == rhs.x && y == rhs.y;
  };

  int distanceSquared(const Position &other) const {
    auto dx = x - other.x;
    auto dy = y - other.y;
    return dx * dx + dy * dy;
  };

  void move(std::array<int, 2> dxy) {
    x += dxy[0];
    y += dxy[1];
  };

  int x;
  int y;
};

enum class RenderOrder /*: uint8_t*/ {
  Corpse,
  Item,
  Actor,
};
extern const std::vector<RenderOrder> allRenderOrders;

struct Renderable {
  int32_t ch;
  tcod::ColorRGB color;
  RenderOrder layer;

  void render(tcod::Console &console, const Position &pos) const;
};
// static_assert(sizeof(Renderable) == 8, "Renderable has the wrong size");

struct Named {
  std::string name;
};

struct BlocksMovement {};

struct Fighter {
  Fighter() : Fighter(0, 0, 0){};
  Fighter(int hp, int defense, int power)
      : max_hp(hp), _hp(hp), base_defense(defense), base_power(power){};

  int hp(void) const { return _hp; }
  void set_hp(int value, flecs::entity self);
  int heal(int amount, flecs::entity self);
  void take_damage(int amount, flecs::entity self);
  void die(flecs::entity self);
  int defense(flecs::entity self) const;
  int power(flecs::entity self) const;

  int max_hp;
  int _hp;
  int base_defense;
  int base_power;
};
