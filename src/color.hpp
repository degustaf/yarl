#pragma once

#include <libtcod.hpp>

namespace color {
constexpr auto white = tcod::ColorRGB{0xFF, 0xFF, 0xFF};
constexpr auto black = tcod::ColorRGB{0, 0, 0};

constexpr auto playerAtk = tcod::ColorRGB{0xE0, 0xE0, 0xE0};
constexpr auto EnemyAtk = tcod::ColorRGB{0xFF, 0x30, 0x30};

constexpr auto welcomeText = tcod::ColorRGB{0x20, 0xA0, 0xFF};

constexpr auto barText = white;
constexpr auto barFilled = tcod::ColorRGB{0, 0x60, 0};
constexpr auto barEmpty = tcod::ColorRGB{0x40, 0x10, 0x10};
}; // namespace color
