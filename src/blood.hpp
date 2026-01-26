#pragma once

#include <cstdint>

#include "console.hpp"

class BloodDrop {
public:
  BloodDrop(int offset) : x_offset(offset), y_(0.0), dy(0.0) {};
  void update(uint64_t ms);
  void render(Console &console, int y_offset) const;
  int y(void) const { return (int)y_; }

private:
  int x_offset;
  double y_;
  double dy;
};
