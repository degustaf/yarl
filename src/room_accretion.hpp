#pragma once

#include <flecs.h>

#include "game_map.hpp"

GameMap generateDungeon(int width, int height, flecs::entity player);
