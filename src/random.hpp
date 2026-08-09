#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ctime>

struct Random {
  Random(uint32_t seed) {
    uint32_t s = seed;
    for (int i = 0; i < Q_SIZE; ++i)
      Q[i] = s = (s * m) + b;
    c = ((s * m) + b) % divisor;
    cur = 0;
  };

  static inline Random *getInstance() {
    static Random *instance = new Random();
    return instance;
  }
  uint32_t getU32(void);
  int getInt(int min, int max);
  template <typename T> inline T get() {
    return T(getU32()) * (T(1.0) / T(0xffffffff));
  }
  float getFloat(float min, float max);
  double getDouble(double min, double max);

private:
  static const auto constexpr Q_SIZE = 4096;
  static const auto constexpr m = 1103515245;
  static const auto constexpr b = 12345;
  static const auto constexpr divisor = 809430660;

  Random() : Random((uint32_t)time(0)) {};

  uint32_t Q[Q_SIZE];
  uint32_t c;
  int cur;
};

template <typename _RandomAccessIterator, typename _RandomNumberGenerator>
void random_shuffle(_RandomAccessIterator __first, _RandomAccessIterator __last,
                    _RandomNumberGenerator &&__rand) {
  if (__first == __last)
    return;
  for (auto __i = __first + 1; __i != __last; ++__i) {
    auto __j = __first + __rand((__i - __first) + 1);
    if (__i != __j)
      std::iter_swap(__i, __j);
  }
}

template <int N> struct Noise {

  Noise() {
    auto rng = Random::getInstance();
    for (auto i = 0; i < 256; i++) {
      map[i] = (uint8_t)i;
      float magnitude = 0.0f;
      for (auto j = 0; j < N; j++) {
        buffer[i][j] = rng->getFloat(-0.5f, 0.5f);
        magnitude += buffer[i][j] * buffer[i][j];
      }

      magnitude = 1.0f / std::sqrt(magnitude);
      for (auto j = 0; j < N; j++) {
        buffer[i][j] *= magnitude;
      }
    }

    for (auto i = 255; i >= 0; i--) {
      auto j = rng->getInt(0, 255);
      std::swap(map[i], map[j]);
    }
  };
  float get(const float (&pt)[N]);

private:
  float lattice(const int (&n)[N], const float (&f)[N]) const {
    int nIndex = 0;
    for (int i = 0; i < N; ++i) {
      nIndex = map[(nIndex + n[i]) & 0xFF];
    }
    float value = 0;
    for (int i = 0; i < N; ++i) {
      value += buffer[nIndex][i] * f[i];
    }
    return value;
  }

  uint8_t map[256];
  float buffer[256][N];
};
