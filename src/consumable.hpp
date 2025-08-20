#pragma once

#include <flecs.h>

#include "action.hpp"

struct HealingConsumable {
  int amount;

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
