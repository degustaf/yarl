#pragma once

#include <flecs.h>
#include <libtcod.hpp>

#include <array>

#include "console.hpp"
#include "game_map.hpp"

void renderBar(Console &console, int currentValue, int maxValue, int x, int y,
               int totalWidth);

void renderSmell(Console &console, flecs::entity player, int x, int y,
                 int totalWidth);
void renderDungeonLevel(Console &console, int level,
                        std::array<int, 2> location);
void renderNamesAtMouseLocation(Console &console, const std::array<int, 2> &xy,
                                const std::array<int, 2> &mouse_loc,
                                flecs::entity map, const GameMap &gameMap);

void renderCommandButton(Console &console, const std::array<int, 4> &xywh);
