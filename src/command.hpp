#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <map>

#include "macro.inc"

enum struct CommandType {
  SILENT_COMMAND_BUILDER(SILENT_ENUM, COMMA),
  COMMAND_BUILDER(ENUM_BUILDER, COMMA)
};

const char *CommandTypeDescription(CommandType tp);
const char *CommandTypeKey(CommandType tp);

struct Command {
  CommandType type;
  SDL_Keycode ch;

  static void init(void);
  static Command get(SDL_KeyboardEvent &key);
  static bool set(SDL_Scancode code, CommandType tp);
  static bool load(std::filesystem::path fileName);
  static void save(std::filesystem::path fileName);

  static std::map<SDL_Scancode, CommandType> mapping;
};
