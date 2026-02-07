#pragma once

#include <flecs.h>
#include <libtcod.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

template <typename T> struct Pos {
  Pos() : x(0), y(0) {};
  Pos(T x, T y) : x(x), y(y) {};
  Pos(std::array<T, 2> xy) : x(xy[0]), y(xy[1]) {};
  Pos(T xy[2]) : x(xy[0]), y(xy[1]) {};
  operator std::array<T, 2>() const { return {x, y}; };
  operator std::array<size_t, 2>() const { return {(size_t)x, (size_t)y}; };
  inline Pos operator+(const std::array<T, 2> &dxy) const {
    return {x + dxy[0], y + dxy[1]};
  };
  inline Pos operator+(const T (&dxy)[2]) const {
    return {x + dxy[0], y + dxy[1]};
  };
  inline bool operator==(const Pos &rhs) const {
    return x == rhs.x && y == rhs.y;
  };

  T distanceSquared(const Pos &other) const {
    auto dx = x - other.x;
    auto dy = y - other.y;
    return dx * dx + dy * dy;
  };

  void move(std::array<T, 2> dxy) {
    x += dxy[0];
    y += dxy[1];
  };

  T x;
  T y;
};

using Position = Pos<int>;
using FPosition = Pos<float>;

enum class RenderOrder /*: uint8_t*/ {
  Corpse,
  Item,
  Actor,
};
extern const std::vector<RenderOrder> allRenderOrders;

struct Renderable {
  int32_t ch;
  tcod::ColorRGB fg;
  std::optional<tcod::ColorRGB> bg;
  RenderOrder layer;
  bool fovOnly = true;

  void render(tcod::Console &console, const Position &pos, bool inFov) const;
};

struct Named {
  std::string name;
};

struct BlocksMovement {};
struct BlocksFov {};
struct Openable {};
struct Fountain {};

void toggleDoor(flecs::entity door);

struct Fighter {
  Fighter() : Fighter(0, 0, 0) {};
  Fighter(int hp, int defense, int power)
      : max_hp(hp), _hp(hp), base_defense(defense), base_power(power) {};

  int hp(void) const { return _hp; }
  void set_hp(int value, flecs::entity self);
  int heal(int amount, flecs::entity self);
  void take_damage(int amount, flecs::entity self);
  void die(flecs::entity self);
  int defense(flecs::entity self) const;
  int power(flecs::entity self, bool ranged) const;
  bool isAlive() const { return _hp > 0; }

  int max_hp;
  int _hp;
  int base_defense;
  int base_power;
};

struct Regenerator {
  int healTurns;
  int turns = 0;
  void update(flecs::entity self);
};

struct OnDeath {
  virtual void onDeath(flecs::entity self) const = 0;
  virtual ~OnDeath() = default;
};

struct Frozen {
  int turns;

  void update(flecs::entity self);
};
