#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>

#include <array>
#include <iostream>

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

struct ActionWithDirection : Action {
  std::array<int, 2> dxy;

  ActionWithDirection(int dx, int dy) : dxy({dx, dy}){};
  virtual ~ActionWithDirection() override = default;
};

struct MoveAction : ActionWithDirection {
  MoveAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  virtual ~MoveAction() override = default;

  virtual SDL_AppResult perform(flecs::entity e) const override {
    auto &pos = e.get_mut<Position>();
    auto mapEntity = e.world().target<CurrentMap>();
    auto &map = mapEntity.get<GameMap>();
    if (map.inBounds(pos + dxy) && map.isWalkable(pos + dxy)) {
      if (GameMap::get_blocking_entity(mapEntity, pos + dxy) == e.null()) {
        pos.move(dxy);
      }
    }
    return SDL_APP_CONTINUE;
  }
};

struct MeleeAction : ActionWithDirection {
  MeleeAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  virtual ~MeleeAction() override = default;

  virtual SDL_AppResult perform(flecs::entity e) const override {
    auto &pos = e.get_mut<Position>();
    auto mapEntity = e.world().target<CurrentMap>();
    auto target = GameMap::get_blocking_entity(mapEntity, pos + dxy);

    if (target != target.null()) {
      std::cout << "You kick the " << target.get<Named>().name
                << ", much to its annoyance\n";
    }

    return SDL_APP_CONTINUE;
  }
};

struct BumpAction : ActionWithDirection {
  BumpAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  virtual ~BumpAction() override = default;

  virtual SDL_AppResult perform(flecs::entity e) const override {
    auto &pos = e.get_mut<Position>();
    auto mapEntity = e.world().target<CurrentMap>();
    if (GameMap::get_blocking_entity(mapEntity, pos + dxy)) {
      return MeleeAction(dxy[0], dxy[1]).perform(e);
    }
    return MoveAction(dxy[0], dxy[1]).perform(e);
  }
};
