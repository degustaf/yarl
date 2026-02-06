#include "command.hpp"

#include <SDL3/SDL_scancode.h>
#include <cassert>
#include <fstream>

static const char *commandDescription[] = {
    SILENT_COMMAND_BUILDER(SILENT, COMMA),
    COMMAND_BUILDER(DESCRIPTION, COMMA),
};

const char *CommandTypeDescription(CommandType type) {
  assert((int)type >= 0);
  assert((unsigned int)type <
         sizeof(commandDescription) / sizeof(commandDescription[0]));
  return commandDescription[(int)type];
}

const char *CommandTypeKey(CommandType tp) {
  for (auto m : Command::mapping) {
    if (m.second == tp) {
      return SDL_GetKeyName(
          SDL_GetKeyFromScancode(m.first, SDL_KMOD_NONE, true));
    }
  }
  return "";
}

static const char *names[] = {
    SILENT_COMMAND_BUILDER(SILENT_NAME, COMMA),
    COMMAND_BUILDER(ENUM_NAME, COMMA),
};

static const char *getCommandTypeName(CommandType type) {
  assert((int)type >= 0);
  assert((unsigned int)type < sizeof(names) / sizeof(names[0]));
  return names[(int)type];
}

static CommandType getCommandTypeFromName(std::string &name) {
  for (unsigned int i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
    auto nm = names[i];
    if (name == nm) {
      return (CommandType)i;
    }
  }
  return CommandType::NONE;
}

static const struct {
  SDL_Scancode code;
  CommandType tp;
} initialCommands[] = {COMMAND_BUILDER(INIT_BUILDER, COMMA)};

void Command::init(void) {
  mapping.clear();
  for (auto &cmd : initialCommands) {
    mapping[cmd.code] = cmd.tp;
  }
}

Command Command::get(SDL_KeyboardEvent &key) {
  switch (key.scancode) {
  case SDL_SCANCODE_UP:
  case SDL_SCANCODE_KP_8:
    return {CommandType::UP, 0};
  case SDL_SCANCODE_DOWN:
  case SDL_SCANCODE_KP_2:
    return {CommandType::DOWN, 0};
  case SDL_SCANCODE_LEFT:
  case SDL_SCANCODE_KP_4:
    return {CommandType::LEFT, 0};
  case SDL_SCANCODE_RIGHT:
  case SDL_SCANCODE_KP_6:
    return {CommandType::RIGHT, 0};
  case SDL_SCANCODE_KP_7:
    return {CommandType::UL, 0};
  case SDL_SCANCODE_KP_9:
    return {CommandType::UR, 0};
  case SDL_SCANCODE_KP_1:
    return {CommandType::DL, 0};
  case SDL_SCANCODE_KP_3:
    return {CommandType::DR, 0};

  case SDL_SCANCODE_RETURN:
  case SDL_SCANCODE_RETURN2:
  case SDL_SCANCODE_KP_ENTER:
    return {CommandType::ENTER, 0};

  case SDL_SCANCODE_PAGEUP:
    return {CommandType::PAGEUP, 0};
  case SDL_SCANCODE_PAGEDOWN:
    return {CommandType::PAGEDOWN, 0};
  case SDL_SCANCODE_HOME:
    return {CommandType::HOME, 0};
  case SDL_SCANCODE_END:
    return {CommandType::END, 0};

  case SDL_SCANCODE_LSHIFT:
  case SDL_SCANCODE_RSHIFT:
    return {CommandType::SHIFT, 0};
  case SDL_SCANCODE_LCTRL:
  case SDL_SCANCODE_RCTRL:
    return {CommandType::CTRL, 0};
  case SDL_SCANCODE_LALT:
  case SDL_SCANCODE_RALT:
    return {CommandType::ALT, 0};

  case SDL_SCANCODE_ESCAPE:
    return {CommandType::ESCAPE, 0};

  default: {
    auto ch = key.key;
    if (ch < SDLK_A || ch > SDLK_Z) {
      ch = 0;
    }
    auto it = mapping.find(key.scancode);
    if (it != mapping.end()) {
      return {it->second, ch};
    } else {
      return {CommandType::NONE, ch};
    }
    break;
  }
  }

  return {CommandType::NONE, 0};
}

bool Command::set(SDL_Scancode code, CommandType tp) {
  switch (code) {
  case SDL_SCANCODE_UNKNOWN:
  case SDL_SCANCODE_UP:
  case SDL_SCANCODE_DOWN:
  case SDL_SCANCODE_RIGHT:
  case SDL_SCANCODE_LEFT:
  case SDL_SCANCODE_RETURN:
  case SDL_SCANCODE_RETURN2:
  case SDL_SCANCODE_KP_1:
  case SDL_SCANCODE_KP_2:
  case SDL_SCANCODE_KP_3:
  case SDL_SCANCODE_KP_4:
  case SDL_SCANCODE_KP_6:
  case SDL_SCANCODE_KP_7:
  case SDL_SCANCODE_KP_8:
  case SDL_SCANCODE_KP_9:
    return false;
  default: {
    // Make sure we don't already have a SCANCODE that maps to the command.
    for (auto it = mapping.begin(); it != mapping.end(); it++) {
      if (it->second == tp) {
        mapping.erase(it);
        break;
      }
    }

    auto it = mapping.find(code);
    if (it != mapping.end()) {
      // We're already using that SCANCODE.
      // Reassign the old one, so it doesn't get lost.
      for (auto ch = SDL_SCANCODE_A; ch <= SDL_SCANCODE_Z;
           ch = (SDL_Scancode)(((int)ch) + 1)) {
        if (mapping.find(ch) == mapping.end()) {
          mapping[ch] = it->second;
          break;
        }
      }
    }

    // Finally, Why we're really here
    mapping[code] = tp;
    return true;
  }
  }
}

bool Command::load(std::filesystem::path fileName) {
  init();
  auto file = std::ifstream(fileName);
  while (file.good()) {
    std::string sc;
    std::string tp;
    file >> sc >> tp;
    if (file.eof()) {
      break;
    }
    SDL_Keymod mod = SDL_KMOD_NONE;
    auto code = SDL_GetScancodeFromKey(SDL_GetKeyFromName(sc.c_str()), &mod);
    if (!set(code, getCommandTypeFromName(tp))) {
      init();
      return false;
    }
  }
  return true;
}

void Command::save(std::filesystem::path fileName) {
  auto file = std::ofstream(fileName);
  for (auto m : mapping) {
    file << SDL_GetKeyName(SDL_GetKeyFromScancode(m.first, SDL_KMOD_NONE, true))
         << "\t" << getCommandTypeName(m.second) << "\n";
  }
}

std::map<SDL_Scancode, CommandType> Command::mapping;
