#pragma once

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

constexpr auto white = RGB{0xFF, 0xFF, 0xFF};
constexpr auto black = RGB{0, 0, 0};
constexpr auto red = RGB{0xFF, 0, 0};
constexpr auto darkerRed = RGB{0x99, 0, 0};
constexpr auto green = RGB{0, 0xFF, 0};
constexpr auto neonGreen = RGB{0x39, 0xFF, 0x14};
constexpr auto desaturatedGreen = RGB{0x3F, 0x7F, 0x3F};
constexpr auto darkerGreen = RGB{0, 0x7F, 0};
constexpr auto blue = RGB{0, 0, 0xFF};
constexpr auto yellow = RGB{0xFF, 0xFF, 0};
constexpr auto orange = RGB{0xFF, 0x6E, 0};
constexpr auto brown = RGB{0x6A, 0x4A, 0x3A};

constexpr auto lightGrey = RGB{0x9F, 0x9F, 0x9F};
constexpr auto darkGrey = RGB{0x3B, 0x3B, 0x3B};

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
