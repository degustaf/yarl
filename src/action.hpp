#pragma once

#include <SDL3/SDL.h>
#include <flecs.h>

#include <array>

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

  virtual SDL_AppResult perform(flecs::entity e) const override;
};

struct MeleeAction : ActionWithDirection {
  MeleeAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  virtual ~MeleeAction() override = default;

  virtual SDL_AppResult perform(flecs::entity e) const override;
};

struct BumpAction : ActionWithDirection {
  BumpAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  virtual ~BumpAction() override = default;

  virtual SDL_AppResult perform(flecs::entity e) const override;
};

struct WaitAction : Action {
  virtual ~WaitAction() = default;
  virtual SDL_AppResult perform(flecs::entity) const override {
    return SDL_APP_CONTINUE;
  };
};
