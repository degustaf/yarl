#pragma once

#include <filesystem>

#include <flecs.h>
#include <fstream>

#include "input_handler.hpp"
#include "room_accretion.hpp"

struct Seed {
  uint32_t seed;
};

namespace Engine {

void handle_enemy_turns(flecs::world ecs);
void save_as(flecs::world ecs, const std::filesystem::path &file_name);

template <typename T>
bool load(flecs::world ecs, const std::filesystem::path &file_name,
          T &handler) {
  auto input = std::ifstream(file_name);
  if (input.fail()) {
    auto f = [](auto, auto &c) {
      c.print({c.get_width() / 2, c.get_height() / 2}, "No saved game to load.",
              color::white, color::black, Console::Alignment::CENTER);
    };
    make<PopupInputHandler<T, decltype(f)>>(ecs, handler, f);
    return false;
  }

  auto buffer = std::stringstream();
  buffer << input.rdbuf();
  if (ecs.from_json(buffer.str().c_str()) == nullptr) {
    auto f = [](auto, auto &c) {
      c.print({c.get_width() / 2, c.get_height() / 2}, "Failed to load save.",
              color::white, color::black, Console::Alignment::CENTER);
    };
    make<PopupInputHandler<T, decltype(f)>>(ecs, handler, f);
    return false;
  }

  auto currentmap = ecs.lookup("currentMap");
  if (currentmap == currentmap.null()) {
    auto f = [](auto, auto &c) {
      c.print({c.get_width() / 2, c.get_height() / 2}, "Failed to load save.",
              color::white, color::black, Console::Alignment::CENTER);
    };
    make<PopupInputHandler<T, decltype(f)>>(ecs, handler, f);
    return false;
  }
  auto map = currentmap.target<CurrentMap>();
  auto &gamemap = map.get_mut<GameMap>();
  gamemap.init();
  auto player = ecs.lookup("player");
  roomAccretion::generateDungeon(map, gamemap, player, false);
  gamemap.update_fov(player);

  return true;
}

void new_game(flecs::world ecs, int map_width, int map_height);
void clear_game_data(flecs::world ecs);
}; // namespace Engine
