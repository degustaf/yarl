#pragma once

#include <libtcod.hpp>

#include <array>
#include <cstdint>

struct Position {
  Position() : x(0), y(0){};
  Position(int x, int y) : x(x), y(y){};
  Position(std::array<int, 2> xy) : x(xy[0]), y(xy[1]){};
  operator std::array<int, 2>() const { return {x, y}; };
  Position operator+(std::array<int, 2> dxy) const {
    return {x + dxy[0], y + dxy[1]};
  };

  void move(std::array<int, 2> dxy) {
    x += dxy[0];
    y += dxy[1];
  };

  int x;
  int y;
};

struct Renderable {
  int32_t ch;
  tcod::ColorRGB color;
};
