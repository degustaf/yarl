#pragma once

#include <filesystem>

#include <flecs.h>

#include "input_handler.hpp"

struct Seed {
  uint32_t seed;
};

namespace Engine {

void handle_enemy_turns(flecs::world ecs);
void save_as(flecs::world ecs, const std::filesystem::path &file_name);
bool load(flecs::world ecs, const std::filesystem::path &file_name,
          MainMenuInputHandler &handler);
void new_game(flecs::world ecs, int map_width, int map_height);
void clear_game_data(flecs::world ecs);
}; // namespace Engine
