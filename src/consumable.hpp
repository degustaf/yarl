#pragma once

#include <flecs.h>

#include "action.hpp"

struct HealingConsumable {
  int amount;

  ActionResult activate(flecs::entity item, flecs::entity target);
};
