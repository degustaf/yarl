#pragma once

#include <array>
#include <string>

#include "color.hpp"
#include "console.hpp"

struct CenterTextBox {
  std::array<int, 2> offset;
  std::string text;
  color::RGB fg;
  Console::Alignment alignment;
};
