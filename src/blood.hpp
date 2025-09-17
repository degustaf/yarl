#pragma once

#include <cstdint>

#include <libtcod.hpp>

class BloodDrop {
public:
  BloodDrop(int x) : x(x), y_(0.0), dy(0.0){};
  void update(uint64_t ms);
  void render(tcod::Console &console, int y_offset) const;
  int y(void) const { return (int)y_; }

private:
  int x;
  double y_;
  double dy;
};
