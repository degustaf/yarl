#pragma once

#include <flecs.h>

struct XP {
  int given;
};

struct Level {
  Level() : current(1), xp(0){};
  int current;
  int xp;

  int xp_to_next_level(void) const;
  inline bool requires_level_up(void) const { return xp > xp_to_next_level(); };
  void add_xp(flecs::world ecs, int new_xp);
  void increaseLevel(void);

  const char *increase_max_hp(flecs::entity e, int amount = 20);
  const char *increase_power(flecs::entity e, int amount = 1);
  const char *increase_defense(flecs::entity e, int amount = 1);
};
