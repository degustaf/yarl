#pragma once

#include <libtcod.h>

#include <array>
#include <string>

struct CenterTextBox {
  std::array<int, 2> offset;
  std::string text;
  TCOD_ColorRGB fg;
  TCOD_alignment_t alignment;
};
