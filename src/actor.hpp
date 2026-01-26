#pragma once

#include <flecs.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "console.hpp"

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

struct MoveAnimation {
  MoveAnimation() : MoveAnimation(0, 0) {};
  MoveAnimation(float x, float y) : x(x), y(y) {};
  MoveAnimation(float x, float y, float speed) : x(x), y(y), speed(speed) {};
  MoveAnimation(const Position &p) : x((float)p.x), y((float)p.y) {};

  operator std::array<float, 2>() const { return {x, y}; };

  float x;
  float y;
  float speed = 0.02f;
};

enum class RenderOrder /*: uint8_t*/ {
  Corpse,
  Item,
  Actor,
};
extern const std::vector<RenderOrder> allRenderOrders;

struct Renderable {
  int32_t ch;
  color::RGB fg;
  std::optional<color::RGB> bg;
  RenderOrder layer;
  bool fovOnly = true;

  void render(Console &console, const Position &pos, bool inFov) const;
  void render(Console &console, const MoveAnimation &pos, bool inFov) const;
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
