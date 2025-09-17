#pragma once

#include <flecs.h>

#include "action.hpp"
#include "scent.hpp"

struct Consumable {};

struct HealingConsumable {
  int amount;

  ActionResult activate(flecs::entity item, flecs::entity target) const;
};

struct DeodorantConsumable {
  float amount;

  ActionResult activate(flecs::entity item, flecs::entity target) const;
};

struct LightningDamageConsumable {
  int damage;
  int maximumRange;

  ActionResult activate(flecs::entity item, flecs::entity consumer) const;
};

struct ConfusionConsumable {
  int number_of_turns;

  ActionResult activate(flecs::entity item) const;
  ActionResult selected(flecs::entity item, flecs::entity consumer,
                        std::array<int, 2> target) const;
};

struct FireballDamageConsumable {
  int damage;
  int radius;

  ActionResult activate(flecs::entity item) const;
  ActionResult selected(flecs::entity item, std::array<int, 2> target) const;
};

struct ScentConsumable {
  Scent scent;

  ActionResult activate(flecs::entity item, flecs::entity consumer) const;
};

struct MagicMappingConsumable {
  ActionResult activate(flecs::entity item, flecs::entity consumer) const;
};

template <typename T> struct TrackerConsumable {
  int turns;
  ActionResult activate(flecs::entity item, flecs::entity consumer) const;
  template <typename Console>
  void render(Console &console, flecs::entity map) const;
};

struct RopeConsumable {
  ActionResult activate(flecs::entity item, flecs::entity consumer) const;
};

struct TransporterConsumable {
  ActionResult activate(flecs::entity item, flecs::entity consumer) const;
};

inline bool isConsumable(flecs::entity e) {
  return e.has<HealingConsumable>() || e.has<LightningDamageConsumable>() ||
         e.has<ConfusionConsumable>() || e.has<FireballDamageConsumable>() ||
         e.has<DeodorantConsumable>() || e.has<ScentConsumable>() ||
         e.has<MagicMappingConsumable>() || /* e.has<TrackerConsumable>() || */
         e.has<RopeConsumable>() || e.has<TransporterConsumable>();
}
