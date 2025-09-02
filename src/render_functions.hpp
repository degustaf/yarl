#pragma once

#include <flecs.h>
#include <libtcod.hpp>

#include <array>

void renderBar(tcod::Console &console, int currentValue, int maxValue,
               int totalWidth);
void renderDungeonLevel(tcod::Console &console, int level,
                        std::array<int, 2> location);
void renderNamesAtMouseLocation(tcod::Console &console,
                                const std::array<int, 2> xy,
                                const std::array<int, 2> &mouse_loc,
                                flecs::entity map);
