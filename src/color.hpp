#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace color {
struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  constexpr inline bool operator==(const RGB &rhs) const {
    return r == rhs.r && g == rhs.g && b == rhs.b;
  };

  constexpr inline bool operator!=(const RGB &rhs) const {
    return !(*this == rhs);
  };

  constexpr inline RGB operator/(const uint8_t rhs) const {
    auto ret = *this;
    ret /= rhs;
    return ret;
  }

  constexpr inline RGB &operator/=(const uint8_t rhs) {
    r /= rhs;
    g /= rhs;
    b /= rhs;
    return *this;
  }

  constexpr inline RGB operator*(float t) {
    return {(uint8_t)std::clamp(r * t, 0.0f, 255.0f),
            (uint8_t)std::clamp(g * t, 0.0f, 255.0f),
            (uint8_t)std::clamp(b * t, 0.0f, 255.0f)};
  }

  constexpr inline RGB operator+(RGB rhs) {
    return {(uint8_t)std::clamp(r + rhs.r, 0, 255),
            (uint8_t)std::clamp(g + rhs.g, 0, 255),
            (uint8_t)std::clamp(b + rhs.b, 0, 255)};
  }
};

struct RGBA {
  constexpr RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : r(r), g(g), b(b), a(a) {};
  constexpr RGBA() : r(0), g(0), b(0), a(255) {};
  constexpr RGBA(RGB c) : r(c.r), g(c.g), b(c.b), a(255) {};
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;

  inline constexpr bool operator==(const RGBA &rhs) const {
    return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
  };

  constexpr inline RGBA &operator+=(const int8_t rhs) {
    r = (uint8_t)std::clamp(r + rhs, 0, 255);
    g = (uint8_t)std::clamp(g + rhs, 0, 255);
    b = (uint8_t)std::clamp(b + rhs, 0, 255);
    return *this;
  }

  constexpr inline RGBA operator/(const uint8_t rhs) const {
    auto ret = *this;
    ret /= rhs;
    return ret;
  }

  constexpr inline RGBA &operator/=(const uint8_t rhs) {
    r /= rhs;
    g /= rhs;
    b /= rhs;
    return *this;
  }
};

static constexpr inline RGB lerp(RGB x, RGB y, float t) {
  return x * t + y * (1 - t);
}

//
// I had no idea what to call lots of colors. There are only so many variations
// of "blueish, I guess", so I sued the color naming tool at
// https://chir.ag/projects/name-that-color
namespace _private {
// shades of grey
constexpr auto white = RGB{0xFF, 0xFF, 0xFF};
constexpr auto altoGrey = RGB{0xDB, 0xDB, 0xDB};
constexpr auto lightGrey = RGB{0x9F, 0x9F, 0x9F};
constexpr auto doveGrey = RGB{0x6C, 0x6C, 0x6C};
constexpr auto darkGrey = RGB{0x3B, 0x3B, 0x3B};
constexpr auto mineShaft = RGB{0x32, 0x32, 0x32};
constexpr auto darkerGrey = RGB{0x19, 0x19, 0x19};
constexpr auto black = RGB{0, 0, 0};

// reds
constexpr auto red = RGB{0xFF, 0, 0};
constexpr auto darkred = RGB{0x99, 0, 0};
constexpr auto maroon = RGB{0x80, 0, 0};
constexpr auto guardsmanRed = RGB{0xBA, 0, 0};
constexpr auto heliotrope = RGB{0xDF, 0x73, 0xFF};

// oranges
constexpr auto redOrange = RGB{0xFF, 0x3F, 0x34};
constexpr auto orange = RGB{0xFF, 0x6E, 0};
constexpr auto sunshade = RGB{0xFF, 0x9E, 0x2C};

// yellows
constexpr auto yellow = RGB{0xFF, 0xFF, 0};
constexpr auto goldenFizz = RGB{0xF5, 0xFB, 0x3D};
constexpr auto earlsGreen = RGB{0xC9, 0xB9, 0x3B}; // Actually a yellow
constexpr auto oldGold = RGB{0xCF, 0xB5, 0x3B};
constexpr auto roti = RGB{0xC6, 0xA8, 0x4B};
constexpr auto pesto = RGB{0x7C, 0x76, 0x31};

// green
constexpr auto green = RGB{0, 0xFF, 0};
constexpr auto goblinGreen = RGB{0x3D, 0x7D, 0x52};
constexpr auto camarone = RGB{0, 0x58, 0x1A};
constexpr auto neonGreen = RGB{0x39, 0xFF, 0x14};
constexpr auto screaminGreen = RGB{0x66, 0xFF, 0x66};
constexpr auto himalaya = RGB{0x6A, 0x5D, 0x1B};
constexpr auto japaneseLaurel = RGB{0x0A, 0x69, 0x06};

// blues
constexpr auto blue = RGB{0, 0, 0xFF};
constexpr auto navyBlue = RGB{0, 0, 0x80};
constexpr auto stratosBlue = RGB{0, 0x07, 0x41};
constexpr auto blueRibbon = RGB{0, 0x66, 0xFF};
constexpr auto cerulean = RGB{0x02, 0xA4, 0xD3};
constexpr auto dodgerBlue = RGB{0x1E, 0x90, 0xFF};
constexpr auto aqua = RGB{0x3F, 0xFF, 0xFF};

// purple
constexpr auto electricViolet = RGB{0x8B, 0, 0xFF};
constexpr auto bunting = RGB{0x15, 0x1F, 0x4C};
constexpr auto minsk = RGB{0x3F, 0x30, 0x7F};

// brown
constexpr auto brown = RGB{0x6A, 0x4A, 0x3A};
constexpr auto paco = RGB{0x41, 0x1F, 0x10};
constexpr auto copperCanyon = RGB{0x7E, 0x3A, 0x15};
}; // namespace _private

constexpr auto text = _private::white;
constexpr auto background = _private::black;
constexpr auto player = _private::navyBlue;
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
