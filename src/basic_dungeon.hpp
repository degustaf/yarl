#pragma once

#include <flecs.h>

#include "game_map.hpp"

namespace basicDungeon {
GameMap generateDungeon(flecs::entity map, int width, int height, int level,
                        flecs::entity player);

void generateDungeon(flecs::entity map, GameMap &dungeon, flecs::entity player,
                     bool generateEntities);
}; // namespace basicDungeon
