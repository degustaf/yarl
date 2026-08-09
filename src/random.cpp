#include "random.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

uint32_t Random::getU32(void) {
  cur = (cur + 1) & (Q_SIZE - 1);
  auto t = (uint64_t)(((uint64_t)187822) * Q[cur] + c);
  c = (uint32_t)(t >> 32);
  auto x = (uint32_t)(t + c);
  if (x < c) {
    x++;
    c++;
  }
  if ((x + 1) == 0) {
    c++;
    x = 0;
  }
  Q[cur] = 0xfffffffe - x;
  return Q[cur];
}

int Random::getInt(int min, int max) {
  if (min == max)
    return min;
  assert(min < max);
  auto delta = max - min + 1;
  auto ret = (int)(getU32() % (uint32_t)delta) + min;
  assert(min <= ret);
  assert(ret <= max);
  return ret;
}

float Random::getFloat(float min, float max) {
  if (min == max)
    return min;
  assert(min < max);
  auto delta = max - min;
  return get<float>() * delta + min;
}

double Random::getDouble(double min, double max) {
  if (min == max)
    return min;
  assert(min < max);
  auto delta = max - min;
  return get<double>() * delta + min;
}

static inline float cubic(float x) { return (3 - 2 * x) * x * x; }

template <typename T> static inline T lerp(T a, T b, T x) {
  return a + x * (b - a);
}

template <> float Noise<2>::get(const float (&pt)[2]) {
  int n[2];
  float r[2];
  float w[2];

  for (int i = 0; i < 2; i++) {
    n[i] = (int)std::floor(pt[i]);
    r[i] = pt[i] - (float)n[i];
    w[i] = cubic(r[i]);
  }

  auto value = lerp(
      lerp(lattice(n, r), lattice({n[0] + 1, n[1]}, {r[0] - 1, r[1]}), w[0]),
      lerp(lattice({n[0], n[1] + 1}, {r[0], r[1] - 1}),
           lattice({n[0] + 1, n[1] + 1}, {r[0] - 1, r[1] - 1}), w[0]),
      w[1]);

  return std::clamp(value, -1.0f, 1.0f);
}

template <> float Noise<3>::get(const float (&pt)[3]) {
  int n[3];
  float r[3];
  float w[3];

  for (int i = 0; i < 3; i++) {
    n[i] = (int)std::floor(pt[i]);
    r[i] = pt[i] - (float)n[i];
    w[i] = cubic(r[i]);
  }

  auto value = lerp(
      lerp(lerp(lattice(n, r),
                lattice({n[0] + 1, n[1], n[2]}, {r[0] - 1, r[1], r[2]}), w[0]),
           lerp(lattice({n[0], n[1] + 1, n[2]}, {r[0], r[1] - 1, r[2]}),
                lattice({n[0] + 1, n[1] + 1, n[2]}, {r[0] - 1, r[1] - 1, r[2]}),
                w[0]),
           w[1]),

      lerp(lerp(lattice({n[0], n[1], n[2] + 1}, {r[0], r[1], r[2] - 1}),
                lattice({n[0] + 1, n[1], n[2] + 1}, {r[0] - 1, r[1], r[2] - 1}),
                w[0]),
           lerp(lattice({n[0], n[1] + 1, n[2] + 1}, {r[0], r[1] - 1, r[2] - 1}),
                lattice({n[0] + 1, n[1] + 1, n[2] + 1},
                        {r[0] - 1, r[1] - 1, r[2] - 1}),
                w[0]),
           w[1]),
      w[2]);

  return std::clamp(value, -1.0f, 1.0f);
}
