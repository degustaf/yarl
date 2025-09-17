#pragma once

#include <algorithm>
#include <cassert>
#include <string>

#include "actor.hpp"

enum class ScentType { none, player, fiend, decay, MAX };

inline std::string scentName(ScentType t) {
  switch (t) {
  case ScentType::none:
    return "nothing";
  case ScentType::player:
    return "human sweat";
  case ScentType::fiend:
    return "the Fiend";
  case ScentType::decay:
    return "decay";
  case ScentType::MAX:
    break;
  }
  assert(false);
  return "";
}

struct Scent {
  ScentType type = ScentType::none;
  float power = 0.0f;

  float reduce(float amount) {
    auto amountReduced = power;
    power = std::max(0.0f, power - amount);
    amountReduced -= power;
    return amountReduced;
  };

  Scent &operator+=(const Scent &rhs) {
    if (type == rhs.type) {
      power += rhs.power;
      return *this;
    }
    if (rhs.power > power) {
      *this = {rhs.type, rhs.power - power};
    }
    power -= rhs.power;
    return *this;
  };
};

struct ScentWarning {
  bool warned;
};

struct ScentOnDeath : OnDeath {
  ScentOnDeath(ScentType type, float power)
      : OnDeath(), type(type), power(power){};
  ScentType type;
  float power;

  virtual void onDeath(flecs::entity self) const override {
    self.set<Scent>({type, power});
  }
  virtual ~ScentOnDeath() = default;
};

struct Smeller {
  float threshold;

  ScentType detect(Scent scent) {
    if (scent.power > threshold) {
      return scent.type;
    }
    return ScentType::none;
  }
};
