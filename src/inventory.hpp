#pragma once

#include <flecs.h>

struct Inventory {
  int capacity;

  bool hasRoom(flecs::entity e) const;
};

struct ContainedBy {};

void drop(flecs::entity item, flecs::entity wearer);
