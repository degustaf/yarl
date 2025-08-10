#pragma once

#include <flecs.h>

#include "input_handler.hpp"
#include "message_log.hpp"

struct Engine {
  EventHandler eventHandler;
  MessageLog messageLog;

  Engine() = default;
  Engine(const Engine &) = delete;
  Engine &operator=(const Engine &) = delete;
  Engine(Engine &&) = default;
  Engine &operator=(Engine &&) = default;

  void handle_enemy_turns(flecs::world ecs) const;
};
