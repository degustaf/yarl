#pragma once

#include <libtcod.hpp>

void renderBar(tcod::Console &console, int currentValue, int maxValue,
               int totalWidth);
