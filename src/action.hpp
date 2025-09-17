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
  float exertion;
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
  virtual ActionResult perform(flecs::entity) const override {
    return {ActionResultType::ExitGood, "", 0.0f};
  }
};

struct QuitWithoutSavingAction : Action {
  virtual ~QuitWithoutSavingAction() override = default;
  virtual ActionResult perform(flecs::entity) const override {
    return {ActionResultType::ExitBad, "", 0.0f};
  }
};

struct ActionWithDirection : Action {
  std::array<int, 2> dxy;

  ActionWithDirection(int dx, int dy) : dxy({dx, dy}){};
  ActionWithDirection(std::array<int, 2> dxy) : dxy(dxy){};
  virtual ~ActionWithDirection() override = default;
};

struct MoveAction : ActionWithDirection {
  MoveAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  MoveAction(std::array<int, 2> dxy) : ActionWithDirection(dxy){};
  virtual ~MoveAction() override = default;

  virtual ActionResult perform(flecs::entity e) const override;
};

struct MeleeAction : ActionWithDirection {
  MeleeAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  virtual ~MeleeAction() override = default;

  virtual ActionResult perform(flecs::entity e) const override;
};

struct DoorDirectionAction : ActionWithDirection {
  DoorDirectionAction(int dx, int dy) : ActionWithDirection(dx, dy){};
  virtual ~DoorDirectionAction() override = default;

  virtual ActionResult perform(flecs::entity e) const override;
};

struct DoorAction : Action {
  virtual ~DoorAction() override = default;
  virtual ActionResult perform(flecs::entity e) const override;
};

struct BatheAction : Action {
  BatheAction(flecs::entity e) : Action(), fountain(e){};
  virtual ~BatheAction() override = default;
  virtual ActionResult perform(flecs::entity e) const override;

  flecs::entity fountain;
};

struct BumpAction : ActionWithDirection {
  BumpAction(int dx, int dy, int speed)
      : ActionWithDirection(dx, dy), speed(speed){};
  BumpAction(std::array<int, 2> dxy, int speed)
      : ActionWithDirection(dxy), speed(speed){};
  int speed;
  virtual ~BumpAction() override = default;

  virtual ActionResult perform(flecs::entity e) const override;
};

struct WaitAction : Action {
  virtual ~WaitAction() = default;
  virtual ActionResult perform(flecs::entity) const override {
    return {ActionResultType::Success, "", 0.0f};
  };
};

struct ItemAction : Action {
  ItemAction(flecs::entity item) : item(item){};
  flecs::entity item;

  virtual ActionResult perform(flecs::entity e) const override;
  virtual ~ItemAction() = default;
};

struct PickupAction : Action {
  virtual ActionResult perform(flecs::entity e) const override;
  virtual ~PickupAction() = default;
};

struct DropItemAction : ItemAction {
  DropItemAction(flecs::entity item) : ItemAction(item){};
  virtual ActionResult perform(flecs::entity e) const override;
  virtual ~DropItemAction() = default;
};

struct TargetedItemAction : ItemAction {
  TargetedItemAction(flecs::entity item, std::array<int, 2> xy)
      : ItemAction(item), target(xy){};
  std::array<int, 2> target;

  virtual ActionResult perform(flecs::entity e) const override;
  virtual ~TargetedItemAction() = default;
};

struct MessageAction : Action {
  MessageAction(std::string msg, tcod::ColorRGB fg = color::white)
      : msg(msg), fg(fg){};
  std::string msg;
  tcod::ColorRGB fg;

  virtual ActionResult perform(flecs::entity e) const override;
  virtual ~MessageAction() = default;
};

struct TakeStairsAction : Action {
  virtual ActionResult perform(flecs::entity e) const;
  virtual ~TakeStairsAction() = default;
};

struct JumpAction : Action {
  JumpAction(bool useRope) : useRope(useRope){};
  bool useRope;
  virtual ActionResult perform(flecs::entity e) const;
  virtual ~JumpAction() = default;
};

struct EquipAction : Action {
  EquipAction(flecs::entity item) : item(item){};
  flecs::entity item;

  virtual ActionResult perform(flecs::entity e) const;
  virtual ~EquipAction() = default;
};

struct RangedTargetAction : Action {
  virtual ActionResult perform(flecs::entity e) const;
  virtual ~RangedTargetAction() = default;
};
