#pragma once

#include <flecs.h>

#include "action.hpp"
#include "scent.hpp"
#include "util.hpp"

struct Consumable {
  virtual ~Consumable() = default;
  virtual ActionResult activate(flecs::entity item,
                                flecs::entity target) const = 0;
};

struct HealingConsumable : Consumable {
  HealingConsumable() = default;
  HealingConsumable(int amount) : amount(amount) {};
  virtual ~HealingConsumable() = default;
  int amount;

  virtual ActionResult activate(flecs::entity item,
                                flecs::entity target) const override;
};

struct DeodorantConsumable : Consumable {
  DeodorantConsumable() = default;
  DeodorantConsumable(float amount) : amount(amount) {};
  virtual ~DeodorantConsumable() = default;
  float amount;

  virtual ActionResult activate(flecs::entity item,
                                flecs::entity target) const override;
};

struct LightningDamageConsumable : Consumable {
  LightningDamageConsumable() = default;
  LightningDamageConsumable(int damage, int maximumRange)
      : damage(damage), maximumRange(maximumRange) {};
  virtual ~LightningDamageConsumable() = default;
  int damage;
  int maximumRange;

  virtual ActionResult activate(flecs::entity item,
                                flecs::entity consumer) const override;
};

struct ConfusionConsumable : Consumable {
  ConfusionConsumable() = default;
  ConfusionConsumable(int number_of_turns)
      : number_of_turns(number_of_turns) {};
  virtual ~ConfusionConsumable() = default;
  int number_of_turns;

  virtual ActionResult activate(flecs::entity item,
                                flecs::entity consumer) const override;
  ActionResult selected(flecs::entity item, flecs::entity consumer,
                        std::array<int, 2> target) const;
};

struct FireballDamageConsumable : Consumable {
  FireballDamageConsumable() = default;
  FireballDamageConsumable(int damage, int radius)
      : damage(damage), radius(radius) {};
  virtual ~FireballDamageConsumable() = default;
  int damage;
  int radius;

  virtual ActionResult activate(flecs::entity item,
                                flecs::entity consumer) const override;
  ActionResult selected(flecs::entity item, std::array<int, 2> target) const;
};

struct ScentConsumable : Consumable {
  ScentConsumable() = default;
  ScentConsumable(Scent scent) : scent(scent) {};
  virtual ~ScentConsumable() = default;
  Scent scent;

  virtual ActionResult activate(flecs::entity item,
                                flecs::entity consumer) const override;
};

struct MagicMappingConsumable : Consumable {
  virtual ActionResult activate(flecs::entity item,
                                flecs::entity consumer) const override;
  virtual ~MagicMappingConsumable() = default;
};

template <typename T> struct TrackerConsumable : Consumable {
  TrackerConsumable<T>() = default;
  TrackerConsumable<T>(int turns) : turns(turns){};
  virtual ~TrackerConsumable<T>() = default;
  int turns;
  virtual ActionResult activate(flecs::entity item,
                                flecs::entity consumer) const override;
  void render(Console &console, flecs::entity map) const;
};

struct RopeConsumable : Consumable {
  virtual ~RopeConsumable() = default;
  virtual ActionResult activate(flecs::entity item,
                                flecs::entity consumer) const override;
};

struct TransporterConsumable : Consumable {
  virtual ~TransporterConsumable() = default;
  virtual ActionResult activate(flecs::entity item,
                                flecs::entity consumer) const override;
};

struct LightConsumable : Consumable {
  LightConsumable() = default;
  LightConsumable(int turns, int innerRadius, int outerRadius,
                  float decayFactor)
      : turns(turns), innerRadius(innerRadius), outerRadius(outerRadius),
        decayFactor(decayFactor) {};
  int turns;
  int innerRadius;
  int outerRadius;
  float decayFactor;
  virtual ~LightConsumable() = default;
  virtual ActionResult activate(flecs::entity item,
                                flecs::entity target) const override;
};

inline bool isConsumable(flecs::entity e) {
  return get<Consumable>(e) != nullptr;
}
