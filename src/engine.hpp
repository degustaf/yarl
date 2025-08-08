#pragma once

#include <flecs.h>

#include "input_handler.hpp"

struct Engine {
  EventHandler eventHandler;

  Engine() : eventHandler(){};
  Engine(const Engine &) = delete;
  Engine &operator=(const Engine &) = delete;
  Engine(Engine &&) = default;
  Engine &operator=(Engine &&) = default;

  void render(flecs::world ecs) const;
  void handle_enemy_turns(flecs::world ecs) const;
};
