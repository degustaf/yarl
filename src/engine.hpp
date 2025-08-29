#pragma once

#include <string>

#include <flecs.h>

#include "input_handler.hpp"

namespace Engine {

void handle_enemy_turns(flecs::world ecs);
void save_as(flecs::world ecs, const std::string &file_name);
bool load(flecs::world ecs, const std::string &file_name,
          EventHandler &eventHandler);

void new_game(flecs::world ecs);
}; // namespace Engine
