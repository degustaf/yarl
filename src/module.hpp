#pragma once

#include <flecs.h>

#include "color.hpp"

struct Module {
  Module(flecs::world ecs);
};

struct QueryModule {
  QueryModule(flecs::world ecs) { ecs.module<QueryModule>("Queries"); }
};

struct Colors {
  Colors(flecs::world ecs);

  static inline color::RGBA text;
  static inline color::RGBA background;
  static inline color::RGBA impossible;
  static inline color::RGBA playerAtk;
  static inline color::RGBA monsterAtk;
  static inline color::RGBA playerDie;
  static inline color::RGBA monsterDie;
  static inline color::RGBA descend;
  static inline color::RGBA healthRecovered;
  static inline color::RGBA needsTarget;
  static inline color::RGBA statusEffectApplied;
  static inline color::RGBA welcomeText;
  static inline color::RGBA jump;

  static inline color::RGBA menu_border;
  static inline color::RGBA menu_background;
  static inline color::RGBA menu_title;
  static inline color::RGBA menu_text;

  static inline color::RGBA barText;
  static inline color::RGBA barFilled;
  static inline color::RGBA barEmpty;

  static inline color::RGBA go;
  static inline color::RGBA caution;
  static inline color::RGBA extraCaution;
  static inline color::RGBA stop;

  static inline color::RGBA dryFountain;
  static inline color::RGBA blood;
  static inline color::RGBA dung;
};
