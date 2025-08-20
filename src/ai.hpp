#pragma once

#include <array>
#include <memory>

#include <flecs.h>
#include <vector>

#include "action.hpp"

struct Ai {
  virtual std::unique_ptr<Action> act(flecs::entity self) = 0;
  virtual ~Ai() = default;

  int unused; // We need this struct to have size > 0 in order to store it
              // in flecs::world.
};

inline bool isAlive(flecs::entity e) { return e.has<Ai>(); }

struct HostileAi : Ai {
  virtual std::unique_ptr<Action> act(flecs::entity self) override;
  virtual ~HostileAi() = default;

  std::vector<std::array<size_t, 2>> path;
};

struct ConfusedAi : Ai {
  ConfusedAi(int turns_remaining) : turns_remaining(turns_remaining){};
  virtual std::unique_ptr<Action> act(flecs::entity self) override;
  virtual ~ConfusedAi() = default;

  int turns_remaining;
};
