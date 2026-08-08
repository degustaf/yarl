#pragma once

#include <algorithm>

#include <libtcod.hpp>

struct Random {
  Random(uint32_t seed) : rng(seed) {};

  static inline Random *getInstance() {
    static Random *instance = new Random();
    return instance;
  }
  inline int getInt(int min, int max) { return rng.getInt(min, max); }
  inline float getFloat(float min, float max) { return rng.getFloat(min, max); }
  inline double getDouble(double min, double max) {
    return rng.getDouble(min, max);
  }

private:
  Random() : rng() {};
  TCODRandom rng;
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

  Noise() : noise(N, N == 2 ? TCOD_NOISE_PERLIN : TCOD_NOISE_DEFAULT) {};
  inline float get(const float (&pt)[N]) { return noise.get(pt); }

private:
  TCODNoise noise;
};
