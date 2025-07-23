#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <flecs.h>

#include "actor.hpp"
#include "game_map.hpp"

struct Action {
  virtual SDL_AppResult perform(flecs::entity e) const = 0;
  virtual ~Action() = default;
};

struct ExitAction : Action {
  virtual ~ExitAction() override = default;
  virtual SDL_AppResult
  perform([[maybe_unused]] flecs::entity e) const override {
    return SDL_APP_SUCCESS;
  }
};

struct MoveAction : Action {
  std::array<int, 2> dxy;

  MoveAction(int x, int y) : dxy({x, y}){};
  virtual ~MoveAction() override = default;

  virtual SDL_AppResult perform(flecs::entity e) const override {
    auto &pos = e.get_mut<Position>();
    if (e.world().target<CurrentMap>().get<GameMap>().isWalkable(pos + dxy)) {
      pos.move(dxy);
    }
    return SDL_APP_CONTINUE;
  }
};
