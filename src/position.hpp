#pragma once

#include <array>
#include <cstddef>

template <typename T> struct Pos {
  Pos() : x(0), y(0) {};
  Pos(T x, T y) : x(x), y(y) {};
  Pos(std::array<T, 2> xy) : x(xy[0]), y(xy[1]) {};
  Pos(T xy[2]) : x(xy[0]), y(xy[1]) {};
  operator std::array<T, 2>() const { return {x, y}; };
  operator std::array<size_t, 2>() const { return {(size_t)x, (size_t)y}; };
  inline Pos operator+(const std::array<T, 2> &dxy) const {
    return {x + dxy[0], y + dxy[1]};
  };
  inline Pos operator+(const T (&dxy)[2]) const {
    return {x + dxy[0], y + dxy[1]};
  };
  inline Pos &operator+=(const std::array<T, 2> &dxy) {
    x += dxy[0];
    y += dxy[1];
    return *this;
  };
  inline Pos &operator+=(const T (&dxy)[2]) const {
    x += dxy[0];
    y += dxy[1];
    return *this;
  };
  inline bool operator==(const Pos &rhs) const {
    return x == rhs.x && y == rhs.y;
  };
  inline bool operator!=(const Pos &rhs) const {
    return x != rhs.x || y != rhs.y;
  };

  T distanceSquared(const Pos &other) const {
    auto dx = x - other.x;
    auto dy = y - other.y;
    return dx * dx + dy * dy;
  };

  void move(std::array<T, 2> dxy) {
    x += dxy[0];
    y += dxy[1];
  };

  T x;
  T y;
};

using Position = Pos<int>;
using FPosition = Pos<float>;

struct Velocity {
  operator std::array<float, 2>() const { return {dx, dy}; };
  Velocity operator*(float dt) const { return {dx * dt, dy * dt}; };

  float dx;
  float dy;
};

struct RadialLimit {
  FPosition center;
  float radius;
};

struct AttackAnimation {
  AttackAnimation(const Position &target, const Position &current,
                  float speed = 0.02f)
      : targetX((float)target.x), targetY((float)target.y), x((float)current.x),
        y((float)current.y), speed(speed) {};

  operator std::array<float, 2>() const { return {x, y}; };
  float distanceTargetSquared() const {
    auto dx = x - targetX;
    auto dy = y - targetY;
    return dx * dx + dy * dy;
  }

  float targetX;
  float targetY;

  float x;
  float y;
  float speed;
};

struct MoveAnimation {
  MoveAnimation() : x(0.0f), y(0.0f) {};
  MoveAnimation(float x, float y) : x(x), y(y) {};
  MoveAnimation(float x, float y, float speed) : x(x), y(y), speed(speed) {};
  MoveAnimation(const Position &p) : x((float)p.x), y((float)p.y) {};
  MoveAnimation(const AttackAnimation &a)
      : x(a.targetX), y(a.targetY), speed(a.speed) {};

  operator std::array<float, 2>() const { return {x, y}; };
  template <typename T> float distanceSquared(const Pos<T> &other) const {
    auto dx = x - (float)other.x;
    auto dy = y - (float)other.y;
    return dx * dx + dy * dy;
  }

  float x;
  float y;
  float speed = 0.02f;
};

struct Fade {
  float delay;
  float fade; // da/ms
};

struct DisappearOnHit {};

struct Trauma {
  float trauma;
};
