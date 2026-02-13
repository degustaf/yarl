#pragma once

#include <algorithm>
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
//
// I had no idea what to call lots of colors. There are only so many variations
// of "blueish, I guess", so I sued the color naming tool at
// https://chir.ag/projects/name-that-color
constexpr auto white = RGB{0xFF, 0xFF, 0xFF};
constexpr auto black = RGB{0, 0, 0};
constexpr auto red = RGB{0xFF, 0, 0};
constexpr auto darkred = RGB{0x99, 0, 0};
constexpr auto redOrange = RGB{0xFF, 0x3F, 0x34};
constexpr auto orange = RGB{0xFF, 0x6E, 0};
constexpr auto guardsmanRed = RGB{0xBA, 0, 0};
constexpr auto maroon = RGB{0x80, 0, 0};
constexpr auto green = RGB{0, 0xFF, 0};
constexpr auto goblinGreen = RGB{0x3D, 0x7D, 0x52};
constexpr auto neonGreen = RGB{0x39, 0xFF, 0x14};
constexpr auto desaturatedGreen = RGB{0x3F, 0x7F, 0x3F};
constexpr auto darkerGreen = RGB{0, 0x7F, 0};
constexpr auto yellow = RGB{0xFF, 0xFF, 0};
constexpr auto brown = RGB{0x6A, 0x4A, 0x3A};
constexpr auto electricViolet = RGB{0x8B, 0, 0xFF};

constexpr auto blue = RGB{0, 0, 0xFF};
constexpr auto navyBlue = RGB{0, 0, 0x80};
constexpr auto stratosBlue = RGB{0, 0x07, 0x41};
constexpr auto blueRibbon = RGB{0, 0x66, 0xFF};
constexpr auto cerulean = RGB{0x02, 0xA4, 0xD3};

constexpr auto lightGrey = RGB{0x9F, 0x9F, 0x9F};
constexpr auto doveGrey = RGB{0x6C, 0x6C, 0x6C};
constexpr auto darkGrey = RGB{0x3B, 0x3B, 0x3B};
constexpr auto mineShaft = RGB{0x32, 0x32, 0x32};
constexpr auto darkerGrey = RGB{0x19, 0x19, 0x19};

constexpr auto himalaya = RGB{0x6A, 0x5D, 0x1B};
constexpr auto roti = RGB{0xC6, 0xA8, 0x4B};
constexpr auto oldGold = RGB{0xCF, 0xB5, 0x3B};
constexpr auto bunting = RGB{0x15, 0x1F, 0x4C};
constexpr auto minsk = RGB{0x3F, 0x30, 0x7F};
constexpr auto pesto = RGB{0x7C, 0x76, 0x31};
constexpr auto japaneseLaurel = RGB{0x0A, 0x69, 0x06};
constexpr auto heliotrope = RGB{0xDF, 0x73, 0xFF};
constexpr auto copperCanyon = RGB{0x7E, 0x3A, 0x15};

constexpr auto playerAtk = RGB{0xE0, 0xE0, 0xE0};
constexpr auto enemyAtk = RGB{0xFF, 0x30, 0x30};
constexpr auto needsTarget = RGB{0x3F, 0xFF, 0xFF};
constexpr auto statusEffectApplied = RGB{0x3F, 0xFF, 0x3F};
constexpr auto descend = RGB{0x9F, 0x3F, 0xFF};

constexpr auto playerDie = RGB{0xFF, 0x30, 0x30};
constexpr auto enemyDie = RGB{0xFF, 0xA0, 0x30};

constexpr auto invalid = color::yellow;
constexpr auto impossible = RGB{0x80, 0x80, 0x80};
constexpr auto error = RGB{0xFF, 0x40, 0x40};

constexpr auto welcomeText = RGB{0x20, 0xA0, 0xFF};
constexpr auto healthRecovered = RGB{0, 0xFF, 0};

constexpr auto barText = white;
constexpr auto barFilled = RGB{0, 0x60, 0};
constexpr auto barEmpty = RGB{0x40, 0x10, 0x10};

constexpr auto menu_title = RGB{0xFF, 0xFF, 0x3F};
constexpr auto menu_text = white;
constexpr auto menu_border = RGB{0xC8, 0xB4, 0x32};
}; // namespace color
