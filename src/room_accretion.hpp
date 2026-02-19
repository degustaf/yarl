#pragma once

#include <flecs.h>

#include "game_map.hpp"

namespace roomAccretion {
struct Config {
  bool FORCE_CONNECTED = true;
  bool lit = true;
  int ROOM_MAX_SIZE = 9;
  int ROOM_MIN_SIZE = 4;
  int MAX_ITER = 600;
  int CELLULAR_AUTOMATA_ITER = 5;
  int STAY_WALL = 4;
  int BECOME_WALL = 5;
  int MIN_LAKE_DIMENSION = 4;
  int MAX_LAKE_WIDTH = 30;
  int MAX_LAKE_HEIGHT = 20;
  int PORTALS = 1;
  int LOOP_ITER = 5;
  int LAKE_ITER = 10;
  size_t MAX_ROOMS = 30;
  size_t MIN_LOOP_DISTANCE = 15;
  double CORRIDOR_PERCENT = 0.50;
  double CELLULAR_AUTOMATA_PERCENT = 0.55;
  double LIGHT_PERCENT = 0.75;
  double FOUNTAIN_PERCENT = 0.15;
  double DOOR_PERCENTAGE = 0.50;
};

GameMap generateDungeon(const Config &cfg, flecs::entity map, int width,
                        int height, int level, flecs::entity player);

void generateDungeon(const Config &cfg, flecs::entity map, GameMap &dungeon,
                     flecs::entity player, bool generateEntities);
}; // namespace roomAccretion
