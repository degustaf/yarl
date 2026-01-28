#pragma once

#include <flecs.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "console.hpp"
#include "position.hpp"

enum class RenderOrder /*: uint8_t*/ {
  Corpse,
  Item,
  Actor,
};
extern const std::vector<RenderOrder> allRenderOrders;

struct Renderable {
  int32_t ch;
  color::RGBA fg;
  std::optional<color::RGB> bg;
  RenderOrder layer;
  float scale = 1.0f;
  bool fovOnly = true;
  bool flipped = false;

  void render(Console &console, const Position &pos, bool inFov) const;
  void render(Console &console, const FPosition &pos, bool inFov) const;
  void render(Console &console, const MoveAnimation &pos, bool inFov) const;
};

struct Named {
  std::string name;
};

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
