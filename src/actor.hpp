#pragma once

#include <flecs.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "console.hpp"
#include "position.hpp"

struct Flying {};
struct Invisible {
  bool paused = false;
};

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

  static constexpr uint8_t darknessFactor = 2;

  template <typename T>
  void render(Console &console, const T &pos, bool inFov, bool invis) const {
    if (fovOnly && !inFov) {
      return;
    }
    auto color = inFov ? fg : (fg / darknessFactor);
    if (invis) {
      color.a /= 2;
    }
    console.addOffGrid(ch, color, (std::array<float, 2>)pos, 1.0f, flipped);
  }
  void render(Console &console, const Position &pos, bool inFov,
              bool invis) const;
};

struct Named {
  std::string name;
};

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

struct Frozen {};

struct Temporary {
  int turns;
  flecs::entity component;

  void update(flecs::entity self);
};
