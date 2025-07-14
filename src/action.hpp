#pragma once

#include <SDL3/SDL.h>

#include <array>

struct Action {
  virtual SDL_AppResult perform(int &x, int &y) const = 0;
  virtual ~Action() = default;
};

struct ExitAction : Action {
  virtual ~ExitAction() override = default;
  virtual SDL_AppResult perform([[maybe_unused]] int &x,
                                [[maybe_unused]] int &y) const override {
    return SDL_APP_SUCCESS;
  }
};

struct MoveAction : Action {
  std::array<int, 2> dxy;

  MoveAction(int x, int y) : dxy({x, y}){};
  virtual ~MoveAction() override = default;

  virtual SDL_AppResult perform(int &x, int &y) const override {
    x += dxy[0];
    y += dxy[1];
    return SDL_APP_CONTINUE;
  }
};
