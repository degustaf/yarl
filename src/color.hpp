#pragma once

#include <libtcod.hpp>

namespace color {
constexpr auto white = tcod::ColorRGB{0xFF, 0xFF, 0xFF};
constexpr auto black = tcod::ColorRGB{0, 0, 0};
constexpr auto red = tcod::ColorRGB{0xFF, 0, 0};
constexpr auto darkerRed = tcod::ColorRGB{0x99, 0, 0};
constexpr auto green = tcod::ColorRGB{0, 0xFF, 0};
constexpr auto neonGreen = tcod::ColorRGB{0x39, 0xFF, 0x14};
constexpr auto desaturatedGreen = tcod::ColorRGB{0x3F, 0x7F, 0x3F};
constexpr auto darkerGreen = tcod::ColorRGB{0, 0x7F, 0};
constexpr auto blue = tcod::ColorRGB{0, 0, 0xFF};
constexpr auto yellow = tcod::ColorRGB{0xFF, 0xFF, 0};
constexpr auto orange = tcod::ColorRGB{0xFF, 0x6E, 0};
constexpr auto lightGrey = tcod::ColorRGB{0x9F, 0x9F, 0x9F};
constexpr auto darkGrey = tcod::ColorRGB{0x3B, 0x3B, 0x3B};
constexpr auto brown = tcod::ColorRGB{0x6A, 0x4A, 0x3A};

constexpr auto playerAtk = tcod::ColorRGB{0xE0, 0xE0, 0xE0};
constexpr auto enemyAtk = tcod::ColorRGB{0xFF, 0x30, 0x30};
constexpr auto needsTarget = tcod::ColorRGB{0x3F, 0xFF, 0xFF};
constexpr auto statusEffectApplied = tcod::ColorRGB{0x3F, 0xFF, 0x3F};
constexpr auto descend = tcod::ColorRGB{0x9F, 0x3F, 0xFF};

constexpr auto playerDie = tcod::ColorRGB{0xFF, 0x30, 0x30};
constexpr auto enemyDie = tcod::ColorRGB{0xFF, 0xA0, 0x30};

constexpr auto invalid = color::yellow;
constexpr auto impossible = tcod::ColorRGB{0x80, 0x80, 0x80};
constexpr auto error = tcod::ColorRGB{0xFF, 0x40, 0x40};

constexpr auto welcomeText = tcod::ColorRGB{0x20, 0xA0, 0xFF};
constexpr auto healthRecovered = tcod::ColorRGB{0, 0xFF, 0};

constexpr auto barText = white;
constexpr auto barFilled = tcod::ColorRGB{0, 0x60, 0};
constexpr auto barEmpty = tcod::ColorRGB{0x40, 0x10, 0x10};

constexpr auto menu_title = tcod::ColorRGB{0xFF, 0xFF, 0x3F};
constexpr auto menu_text = white;
constexpr auto menu_border = tcod::ColorRGB{0xC8, 0xB4, 0x32};
}; // namespace color

static inline TCOD_ColorRGBA operator/(TCOD_ColorRGBA lhs, const uint8_t rhs) {
  auto ret = lhs;
  ret.r /= rhs;
  ret.g /= rhs;
  ret.b /= rhs;
  return ret;
}

static inline TCOD_ColorRGBA &operator/=(TCOD_ColorRGBA &lhs,
                                         const uint8_t rhs) {
  lhs.r /= rhs;
  lhs.g /= rhs;
  lhs.b /= rhs;
  return lhs;
}
