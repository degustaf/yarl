#pragma once

#include <cassert>
#include <string>

#include <flecs.h>
#include <libtcod.hpp>

#include "actor.hpp"

struct Inventory {
  int capacity;

  bool hasRoom(flecs::entity e) const;
};

struct ContainedBy {};
struct Item {};

void drop(flecs::entity item, flecs::entity wearer);

enum class EquipmentType { Weapon, Armor };

struct Equippable {
  EquipmentType type;
  int power_bonus;
  int defense_bonus;
};

struct Armor {};
struct Weapon {};

struct Ranged {
  int range;
};

template <typename T, bool print>
static inline std::string unequip(flecs::entity owner, flecs::entity item) {
  owner.remove<T>(item);
  if (print) {
    return tcod::stringf("You remove the %s. ", item.get<Named>().name.c_str());
  } else {
    return "";
  }
}

template <typename T, bool print>
static inline std::string equip(flecs::entity owner, flecs::entity item) {
  auto currentItem = owner.target<T>();
  auto msg = currentItem ? unequip<T, print>(owner, currentItem) : "";

  owner.add<T>(item);
  if (print) {
    return tcod::stringf("%sYou equip the %s.", msg.c_str(),
                         item.get<Named>().name.c_str());
  } else {
    return "";
  }
}

template <typename T, bool print>
static inline std::string toggleEquip(flecs::entity owner, flecs::entity item) {
  if (owner.target<T>() == item) {
    return unequip<T, print>(owner, item);
  }
  return equip<T, print>(owner, item);
}

template <bool print>
std::string toggleEquip(flecs::entity owner, flecs::entity item) {
  switch (item.get<Equippable>().type) {
  case EquipmentType::Weapon:
    return toggleEquip<Weapon, print>(owner, item);
  case EquipmentType::Armor:
    return toggleEquip<Armor, print>(owner, item);
  default:
    assert(false);
    return "";
  }
}

inline bool isEquipped(flecs::entity owner, flecs::entity item) {
  return (owner.target<Weapon>() == item) || (owner.target<Armor>() == item);
}

struct Taser {
  int turns;
  void apply(flecs::entity target) const;
};
