#pragma once

#include <libtcod.hpp>

namespace color {
// I had no idea what to call lots of colors. There are only so many variations
// of "blueish, I guess", so I sued the color naming tool at
// https://chir.ag/projects/name-that-color
constexpr auto white = tcod::ColorRGB{0xFF, 0xFF, 0xFF};
constexpr auto black = tcod::ColorRGB{0, 0, 0};
constexpr auto red = tcod::ColorRGB{0xFF, 0, 0};
constexpr auto darkred = tcod::ColorRGB{0x99, 0, 0};
constexpr auto redOrange = tcod::ColorRGB{0xFF, 0x3F, 0x34};
constexpr auto orange = tcod::ColorRGB{0xFF, 0x6E, 0};
constexpr auto guardsmanRed = tcod::ColorRGB{0xBA, 0, 0};
constexpr auto maroon = tcod::ColorRGB{0x80, 0, 0};
constexpr auto green = tcod::ColorRGB{0, 0xFF, 0};
constexpr auto goblinGreen = tcod::ColorRGB{0x3D, 0x7D, 0x52};
constexpr auto neonGreen = tcod::ColorRGB{0x39, 0xFF, 0x14};
constexpr auto yellow = tcod::ColorRGB{0xFF, 0xFF, 0};
constexpr auto brown = tcod::ColorRGB{0x6A, 0x4A, 0x3A};
constexpr auto electricViolet = tcod::ColorRGB{0x8B, 0, 0xFF};

constexpr auto blue = tcod::ColorRGB{0, 0, 0xFF};
constexpr auto navyBlue = tcod::ColorRGB{0, 0, 0x80};
constexpr auto stratosBlue = tcod::ColorRGB{0, 0x07, 0x41};
constexpr auto blueRibbon = tcod::ColorRGB{0, 0x66, 0xFF};
constexpr auto cerulean = tcod::ColorRGB{0x02, 0xA4, 0xD3};

constexpr auto lightGrey = tcod::ColorRGB{0x9F, 0x9F, 0x9F};
constexpr auto doveGrey = tcod::ColorRGB{0x6C, 0x6C, 0x6C};
constexpr auto darkGrey = tcod::ColorRGB{0x3B, 0x3B, 0x3B};
constexpr auto mineShaft = tcod::ColorRGB{0x32, 0x32, 0x32};
constexpr auto darkerGrey = tcod::ColorRGB{0x19, 0x19, 0x19};

constexpr auto himalaya = tcod::ColorRGB{0x6A, 0x5D, 0x1B};
constexpr auto roti = tcod::ColorRGB{0xC6, 0xA8, 0x4B};
constexpr auto oldGold = tcod::ColorRGB{0xCF, 0xB5, 0x3B};
constexpr auto bunting = tcod::ColorRGB{0x15, 0x1F, 0x4C};
constexpr auto minsk = tcod::ColorRGB{0x3F, 0x30, 0x7F};
constexpr auto pesto = tcod::ColorRGB{0x7C, 0x76, 0x31};
constexpr auto japaneseLaurel = tcod::ColorRGB{0x0A, 0x69, 0x06};
constexpr auto heliotrope = tcod::ColorRGB{0xDF, 0x73, 0xFF};
constexpr auto copperCanyon = tcod::ColorRGB{0x7E, 0x3A, 0x15};

constexpr auto playerAtk = tcod::ColorRGB{0xE0, 0xE0, 0xE0};
constexpr auto enemyAtk = tcod::ColorRGB{0xFF, 0x30, 0x30};
constexpr auto needsTarget = tcod::ColorRGB{0x3F, 0xFF, 0xFF};
constexpr auto statusEffectApplied = tcod::ColorRGB{0x3F, 0xFF, 0x3F};
constexpr auto descend = tcod::ColorRGB{0x9F, 0x3F, 0xFF};

constexpr auto playerDie = tcod::ColorRGB{0xFF, 0x30, 0x30};
constexpr auto enemyDie = tcod::ColorRGB{0xFF, 0xA0, 0x30};

constexpr auto invalid = tcod::ColorRGB{0xFF, 0xFF, 0};
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

static inline TCOD_ColorRGBA &operator+=(TCOD_ColorRGBA &lhs,
                                         const int8_t rhs) {
  lhs.r = (uint8_t)std::clamp(lhs.r + rhs, 0, 255);
  lhs.g = (uint8_t)std::clamp(lhs.g + rhs, 0, 255);
  lhs.b = (uint8_t)std::clamp(lhs.b + rhs, 0, 255);
  return lhs;
}
