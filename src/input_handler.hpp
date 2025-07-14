#pragma once

#include <SDL3/SDL.h>

#include <memory>

#include "action.hpp"

struct EventHandler {
  std::unique_ptr<Action> dispatch(SDL_Event *event) const;
};
