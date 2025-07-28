#include "input_handler.hpp"

std::unique_ptr<Action> EventHandler::dispatch(SDL_Event *event) const {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return std::make_unique<ExitAction>();

  case SDL_EVENT_KEY_DOWN:
    switch (event->key.scancode) {
    case SDL_SCANCODE_UP:
      return std::make_unique<BumpAction>(0, -1);
    case SDL_SCANCODE_DOWN:
      return std::make_unique<BumpAction>(0, 1);
    case SDL_SCANCODE_LEFT:
      return std::make_unique<BumpAction>(-1, 0);
    case SDL_SCANCODE_RIGHT:
      return std::make_unique<BumpAction>(1, 0);

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
