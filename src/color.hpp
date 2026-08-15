#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace color {
struct RGBA {
  constexpr RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : r(r), g(g), b(b), a(a) {};
  constexpr RGBA() : r(0), g(0), b(0), a(255) {};
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

  constexpr inline RGBA operator*(float t) const {
    return {(uint8_t)std::clamp(r * t, 0.0f, 255.0f),
            (uint8_t)std::clamp(g * t, 0.0f, 255.0f),
            (uint8_t)std::clamp(b * t, 0.0f, 255.0f),
            (uint8_t)std::clamp(a * t, 0.0f, 255.0f)};
  }

  constexpr inline RGBA operator*(RGBA rhs) {
    auto af = a / 255.0f;
    auto rhs_af = rhs.a / 255.0f;
    auto a_final = af + rhs_af * (1.0f - af);
    auto ret = *this * af + rhs * rhs_af * (1.0f - af);
    ret = ret * a_final;
    ret.a = (uint8_t)(255 * a_final);
    return ret;
  }

  constexpr inline RGBA operator+(RGBA rhs) {
    return {(uint8_t)std::clamp(r + rhs.r, 0, 255),
            (uint8_t)std::clamp(g + rhs.g, 0, 255),
            (uint8_t)std::clamp(b + rhs.b, 0, 255),
            (uint8_t)std::clamp(a + rhs.a, 0, 255)};
  }
};

template <typename T> static constexpr inline T lerp(T x, T y, float t) {
  return x * t + y * (1 - t);
}
}; // namespace color
