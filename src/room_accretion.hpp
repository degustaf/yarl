#pragma once

#include <flecs.h>

#include "game_map.hpp"

GameMap generateDungeon(flecs::entity map, int width, int height,
                        flecs::entity player);
