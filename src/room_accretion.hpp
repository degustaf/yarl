#pragma once

#include <flecs.h>

#include "game_map.hpp"

namespace roomAccretion {
GameMap generateDungeon(flecs::entity map, int width, int height, int level,
                        flecs::entity player, bool lit);

void generateDungeon(flecs::entity map, GameMap &dungeon, flecs::entity player,
                     bool generateEntities, bool lit);
}; // namespace roomAccretion
