#include "blood.hpp"

#include <cmath>

#include "color.hpp"

static constexpr auto gravity = 0.1;

void BloodDrop::update(uint64_t ms) {
  auto dt = ((double)ms) / 100.0;
  y_ += dy * dt + gravity / 2.0 * dt * dt;
  dy += dt * gravity;
}

void BloodDrop::render(Console &console, int y_offset) const {
  double integer = 0.0;
  double frac = modf(y_, &integer);
  auto ch = y_ < 0.5 ? '`' : y_ < 1.0 ? 0xBF : frac < 0.5 ? '`' : '.';
  auto &tile = console.at({x, y_offset + (int)integer});
  tile.ch = ch;
  tile.fg = color::red;
}
