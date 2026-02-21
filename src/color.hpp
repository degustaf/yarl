#pragma once

#include <cassert>
#include <libtcod.hpp>

namespace color {
// I had no idea what to call lots of colors. There are only so many variations
// of "blueish, I guess", so I sued the color naming tool at
// https://chir.ag/projects/name-that-color
namespace _private {
// shades of grey
constexpr auto white = tcod::ColorRGB{0xFF, 0xFF, 0xFF};
constexpr auto altoGrey = tcod::ColorRGB{0xDB, 0xDB, 0xDB};
constexpr auto lightGrey = tcod::ColorRGB{0x9F, 0x9F, 0x9F};
constexpr auto doveGrey = tcod::ColorRGB{0x6C, 0x6C, 0x6C};
constexpr auto darkGrey = tcod::ColorRGB{0x3B, 0x3B, 0x3B};
constexpr auto mineShaft = tcod::ColorRGB{0x32, 0x32, 0x32};
constexpr auto darkerGrey = tcod::ColorRGB{0x19, 0x19, 0x19};
constexpr auto black = tcod::ColorRGB{0, 0, 0};

// reds
constexpr auto red = tcod::ColorRGB{0xFF, 0, 0};
constexpr auto darkred = tcod::ColorRGB{0x99, 0, 0};
constexpr auto maroon = tcod::ColorRGB{0x80, 0, 0};
constexpr auto guardsmanRed = tcod::ColorRGB{0xBA, 0, 0};
constexpr auto heliotrope = tcod::ColorRGB{0xDF, 0x73, 0xFF};

// oranges
constexpr auto redOrange = tcod::ColorRGB{0xFF, 0x3F, 0x34};
constexpr auto orange = tcod::ColorRGB{0xFF, 0x6E, 0};
constexpr auto sunshade = tcod::ColorRGB{0xFF, 0x9E, 0x2C};

// yellows
constexpr auto yellow = tcod::ColorRGB{0xFF, 0xFF, 0};
constexpr auto goldenFizz = tcod::ColorRGB{0xF5, 0xFB, 0x3D};
constexpr auto earlsGreen =
    tcod::ColorRGB{0xC9, 0xB9, 0x3B}; // Actually a yellow
constexpr auto oldGold = tcod::ColorRGB{0xCF, 0xB5, 0x3B};
constexpr auto roti = tcod::ColorRGB{0xC6, 0xA8, 0x4B};
constexpr auto pesto = tcod::ColorRGB{0x7C, 0x76, 0x31};

// green
constexpr auto green = tcod::ColorRGB{0, 0xFF, 0};
constexpr auto goblinGreen = tcod::ColorRGB{0x3D, 0x7D, 0x52};
constexpr auto camarone = tcod::ColorRGB{0, 0x58, 0x1A};
constexpr auto neonGreen = tcod::ColorRGB{0x39, 0xFF, 0x14};
constexpr auto screaminGreen = tcod::ColorRGB{0x66, 0xFF, 0x66};
constexpr auto himalaya = tcod::ColorRGB{0x6A, 0x5D, 0x1B};
constexpr auto japaneseLaurel = tcod::ColorRGB{0x0A, 0x69, 0x06};

// blues
constexpr auto blue = tcod::ColorRGB{0, 0, 0xFF};
constexpr auto navyBlue = tcod::ColorRGB{0, 0, 0x80};
constexpr auto stratosBlue = tcod::ColorRGB{0, 0x07, 0x41};
constexpr auto blueRibbon = tcod::ColorRGB{0, 0x66, 0xFF};
constexpr auto cerulean = tcod::ColorRGB{0x02, 0xA4, 0xD3};
constexpr auto dodgerBlue = tcod::ColorRGB{0x1E, 0x90, 0xFF};
constexpr auto aqua = tcod::ColorRGB{0x3F, 0xFF, 0xFF};

// purple
constexpr auto electricViolet = tcod::ColorRGB{0x8B, 0, 0xFF};
constexpr auto bunting = tcod::ColorRGB{0x15, 0x1F, 0x4C};
constexpr auto minsk = tcod::ColorRGB{0x3F, 0x30, 0x7F};

// brown
constexpr auto brown = tcod::ColorRGB{0x6A, 0x4A, 0x3A};
constexpr auto paco = tcod::ColorRGB{0x41, 0x1F, 0x10};
constexpr auto copperCanyon = tcod::ColorRGB{0x7E, 0x3A, 0x15};
}; // namespace _private

constexpr auto text = _private::white;
constexpr auto background = _private::black;
constexpr auto playerAtk = _private::altoGrey;
constexpr auto enemyAtk = _private::redOrange;
constexpr auto needsTarget = _private::aqua;
constexpr auto statusEffectApplied = _private::screaminGreen;
constexpr auto descend = _private::electricViolet;
constexpr auto stairs = _private::white;
constexpr auto walls = _private::black;
constexpr auto darkWallbg = _private::navyBlue;
constexpr auto lightWallbg = _private::pesto;
constexpr auto fountain = _private::blue;
constexpr auto dryFountain = _private::lightGrey;
constexpr auto lightFG = _private::himalaya;
constexpr auto lightFloor = _private::oldGold;
constexpr auto darkFG = _private::bunting;
constexpr auto darkFloor = _private::minsk;
constexpr auto sensedFloor = _private::doveGrey;
constexpr auto chasmFG = _private::doveGrey;
constexpr auto chasm = _private::mineShaft;
constexpr auto sensedFG = _private::darkerGrey;
constexpr auto tool = _private::lightGrey;
constexpr auto weapon = _private::cerulean;
constexpr auto laser = _private::orange;
constexpr auto armor = _private::copperCanyon;
constexpr auto jump = _private::red;
constexpr auto blood = _private::red;
constexpr auto areaTarget = _private::red;
constexpr auto fireball = _private::red;
constexpr auto lightning = _private::yellow;
constexpr auto confusion = _private::heliotrope;
constexpr auto door = _private::darkred;
constexpr auto portal = _private::guardsmanRed;
constexpr auto water_fg = _private::blue;
constexpr auto water_bg = _private::blueRibbon;
constexpr auto dark_water_fg = _private::navyBlue;
constexpr auto dark_water_bg = _private::stratosBlue;
constexpr auto deodorant = _private::redOrange;
constexpr auto potion = _private::electricViolet;
constexpr auto sensed = _private::neonGreen;

constexpr auto playerDie = _private::redOrange;
constexpr auto enemyDie = _private::sunshade;

constexpr auto invalid = _private::yellow;
constexpr auto impossible = _private::doveGrey;
constexpr auto error = _private::redOrange;

constexpr auto welcomeText = _private::dodgerBlue;
constexpr auto healthRecovered = _private::green;

constexpr auto barText = _private::white;
constexpr auto barFilled = _private::camarone;
constexpr auto barEmpty = _private::paco;

constexpr auto menu_title = _private::goldenFizz;
constexpr auto menu_text = _private::white;
constexpr auto menu_border = _private::earlsGreen;
constexpr auto menu_background = _private::darkGrey;

constexpr auto go = _private::green;
constexpr auto caution = _private::yellow;
constexpr auto extraCaution = _private::orange;
constexpr auto stop = _private::red;

constexpr auto cyst = _private::black;
constexpr auto orc = _private::goblinGreen;
constexpr auto troll = _private::japaneseLaurel;
constexpr auto dung = _private::brown;
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

static inline TCOD_ColorRGB operator*(float t, TCOD_ColorRGB c) {
  return {(uint8_t)std::clamp(c.r * t, 0.0f, 255.0f),
          (uint8_t)std::clamp(c.g * t, 0.0f, 255.0f),
          (uint8_t)std::clamp(c.b * t, 0.0f, 255.0f)};
}

static inline TCOD_ColorRGB operator+(TCOD_ColorRGB x, TCOD_ColorRGB y) {
  return {(uint8_t)std::clamp(x.r + y.r, 0, 255),
          (uint8_t)std::clamp(x.g + y.g, 0, 255),
          (uint8_t)std::clamp(x.b + y.b, 0, 255)};
}

static inline TCOD_ColorRGB lerp(TCOD_ColorRGB x, TCOD_ColorRGB y, float t) {
  return t * x + (1 - t) * y;
}

static inline TCOD_ColorRGBA operator*(float t, TCOD_ColorRGBA c) {
  return {(uint8_t)std::clamp(c.r * t, 0.0f, 255.0f),
          (uint8_t)std::clamp(c.g * t, 0.0f, 255.0f),
          (uint8_t)std::clamp(c.b * t, 0.0f, 255.0f),
          (uint8_t)std::clamp(c.a * t, 0.0f, 255.0f)};
}

static inline TCOD_ColorRGBA operator+(TCOD_ColorRGBA x, TCOD_ColorRGBA y) {
  return {(uint8_t)std::clamp(x.r + y.r, 0, 255),
          (uint8_t)std::clamp(x.g + y.g, 0, 255),
          (uint8_t)std::clamp(x.b + y.b, 0, 255),
          (uint8_t)std::clamp(x.a + y.a, 0, 255)};
}

static inline TCOD_ColorRGBA lerp(TCOD_ColorRGBA x, TCOD_ColorRGBA y, float t) {
  return t * x + (1 - t) * y;
}
