#pragma once

#include <array>
#include <string>

#include <SDL3/SDL.h>
#include <flecs.h>
#include <libtcod.hpp>

#include "color.hpp"

enum struct ActionResultType {
  Success,
  Failure,
  ExitGood,
  ExitBad,
};

struct ActionResult {
  ActionResultType type;
  std::string msg;
  tcod::ColorRGB fg = color::white;

  inline operator bool() {
    switch (type) {
    case ActionResultType::Success:
      return true;
    case ActionResultType::Failure:
    case ActionResultType::ExitGood:
    case ActionResultType::ExitBad:
      return false;
    }
    return false;
  }

  inline operator SDL_AppResult() {
    switch (type) {
    case ActionResultType::Success:
    case ActionResultType::Failure:
      return SDL_APP_CONTINUE;
    case ActionResultType::ExitGood:
      return SDL_APP_SUCCESS;
    case ActionResultType::ExitBad:
      return SDL_APP_FAILURE;
    }
    return SDL_APP_FAILURE;
  }
};

struct Action {
  virtual ActionResult perform(flecs::entity e) const = 0;
  virtual ~Action() = default;
};

struct ExitAction : Action {
  virtual ~ExitAction() override = default;
  virtual ActionResult
  perform([[maybe_unused]] flecs::entity e) const override {
    return {ActionResultType::ExitGood, ""};
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

  virtual ActionResult perform(flecs::entity e) const override;
};

struct MeleeAction : ActionWithDirection {
  MeleeAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  virtual ~MeleeAction() override = default;

  virtual ActionResult perform(flecs::entity e) const override;
};

struct BumpAction : ActionWithDirection {
  BumpAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  virtual ~BumpAction() override = default;

  virtual ActionResult perform(flecs::entity e) const override;
};

struct WaitAction : Action {
  virtual ~WaitAction() = default;
  virtual ActionResult perform(flecs::entity) const override {
    return {ActionResultType::Success, ""};
  };
};

struct ItemAction : Action {
  flecs::entity item;

  virtual ActionResult perform(flecs::entity e) const override;
  virtual ~ItemAction() = default;
};
