#pragma once

#include <libtcod.hpp>

struct tile {
  bool walkable;
  bool transparent;
  TCOD_ConsoleTile dark;

  static tile floor_tile;
  static tile wall_tile;
};
