#pragma once

#include <libtcod.hpp>

struct tile {
  bool walkable;
  bool transparent;
  TCOD_ConsoleTile dark;
  TCOD_ConsoleTile light;

  static tile floor_tile;
  static tile wall_tile;
  static TCOD_ConsoleTile shroud;
};
