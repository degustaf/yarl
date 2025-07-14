#pragma once

#include <libtcod.hpp>

#include <array>
#include <cstdint>

struct Position {
  int x;
  int y;

  operator std::array<int, 2>() const { return {x, y}; };
  Position operator+(std::array<int, 2> dxy) const {
    return {x + dxy[0], y + dxy[1]};
  };

  void move(std::array<int, 2> dxy) {
    x += dxy[0];
    y += dxy[1];
  };
};

struct Renderable {
  int32_t ch;
  tcod::ColorRGB color;
};
