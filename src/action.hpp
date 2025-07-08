#pragma once

#include <SDL3/SDL.h>

#include <array>

enum struct ActionType {
  QUIT,
  MOVE,
  ESCAPE,

  NONE
};

struct Action {
  ActionType type;
  std::array<int, 2> xy;

  static Action get(const SDL_Event *);
};
