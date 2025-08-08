#include "input_handler.hpp"

std::unique_ptr<Action> MainGameDispatch(SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return std::make_unique<ExitAction>();

  case SDL_EVENT_KEY_DOWN:
    switch (event->key.scancode) {
    case SDL_SCANCODE_UP:
    case SDL_SCANCODE_KP_8:
    case SDL_SCANCODE_K:
      return std::make_unique<BumpAction>(0, -1);
    case SDL_SCANCODE_DOWN:
    case SDL_SCANCODE_KP_2:
    case SDL_SCANCODE_J:
      return std::make_unique<BumpAction>(0, 1);
    case SDL_SCANCODE_LEFT:
    case SDL_SCANCODE_KP_4:
    case SDL_SCANCODE_H:
      return std::make_unique<BumpAction>(-1, 0);
    case SDL_SCANCODE_RIGHT:
    case SDL_SCANCODE_KP_6:
    case SDL_SCANCODE_L:
      return std::make_unique<BumpAction>(1, 0);
    case SDL_SCANCODE_HOME:
    case SDL_SCANCODE_KP_7:
    case SDL_SCANCODE_Y:
      return std::make_unique<BumpAction>(-1, -1);
    case SDL_SCANCODE_END:
    case SDL_SCANCODE_KP_1:
    case SDL_SCANCODE_B:
      return std::make_unique<BumpAction>(-1, 1);
    case SDL_SCANCODE_PAGEUP:
    case SDL_SCANCODE_KP_9:
    case SDL_SCANCODE_U:
      return std::make_unique<BumpAction>(1, -1);
    case SDL_SCANCODE_PAGEDOWN:
    case SDL_SCANCODE_KP_3:
    case SDL_SCANCODE_N:
      return std::make_unique<BumpAction>(1, 1);

    case SDL_SCANCODE_PERIOD:
    case SDL_SCANCODE_KP_5:
    case SDL_SCANCODE_CLEAR:
      return std::make_unique<WaitAction>();

    case SDL_SCANCODE_ESCAPE:
      return std::make_unique<ExitAction>();

    default:
      return nullptr;
    }
    break;

  default:
    return nullptr;
  }
}

std::unique_ptr<Action> GameOverDispatch(SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return std::make_unique<ExitAction>();

  case SDL_EVENT_KEY_DOWN:
    switch (event->key.scancode) {
    case SDL_SCANCODE_ESCAPE:
      return std::make_unique<ExitAction>();

    default:
      return nullptr;
    }

  default:
    return nullptr;
  }
}
