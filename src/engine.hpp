#pragma once

#include <flecs.h>

struct Engine {
  int unused; // We need this struct to have size > 0 in order to store it
              // in flecs::world.

  Engine() : unused(0){};
  Engine(const Engine &) = delete;
  Engine &operator=(const Engine &) = delete;
  Engine(Engine &&) = default;
  Engine &operator=(Engine &&) = default;

  void render(flecs::world ecs) const;
};
