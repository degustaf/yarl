#define SILENT_COMMAND_BUILDER(X, SEP) \
  X(NONE) SEP \
  X(ENTER) SEP \
  X(ESCAPE) SEP \
  X(PAGEUP) SEP \
  X(PAGEDOWN) SEP \
  X(HOME) SEP \
  X(END) SEP \
  X(SHIFT) SEP \
  X(CTRL) SEP \
  X(ALT)

#define SILENT_ENUM(COMMAND) COMMAND
#define SILENT(COMMAND) ""
#define SILENT_NAME(COMMAND) #COMMAND


#define COMMAND_BUILDER(X, SEP) \
  X(AUTO, SDL_SCANCODE_A, "autoexplore") SEP \
  X(DL, SDL_SCANCODE_B, "down-left") SEP \
  X(COMMANDS, SDL_SCANCODE_C, "open command menu") SEP \
  X(DROP, SDL_SCANCODE_D, "drop") SEP \
  X(GRAB, SDL_SCANCODE_G, "grab") SEP \
  X(LEFT, SDL_SCANCODE_H, "left") SEP \
  X(INVENTORY, SDL_SCANCODE_I, "inventory") SEP \
  X(DOWN, SDL_SCANCODE_J, "down") SEP \
  X(UP, SDL_SCANCODE_K, "up") SEP \
  X(RIGHT, SDL_SCANCODE_L, "right") SEP \
  X(MESSAGES, SDL_SCANCODE_M, "expand log") SEP \
  X(DR, SDL_SCANCODE_N, "down-right") SEP \
  X(OPEN, SDL_SCANCODE_O, "open/close a door") SEP \
  X(PEEK, SDL_SCANCODE_P, "peek a tile") SEP \
  X(SHOOT, SDL_SCANCODE_S, "shoot a ranged weapon") SEP \
  X(TURN, SDL_SCANCODE_T, "display turn and seed") SEP \
  X(UR, SDL_SCANCODE_U, "up-right") SEP \
  X(CHARACTER, SDL_SCANCODE_X, "character screen") SEP \
  X(UL, SDL_SCANCODE_Y, "up-left") SEP \
  X(HUD, SDL_SCANCODE_Z, "toggle hud") SEP \
  X(WAIT, SDL_SCANCODE_SPACE, "wait a turn") SEP \
  X(STAIRS, SDL_SCANCODE_PERIOD, "use the stairs")

  // X(EQUIP, SDL_SCANCODE_E, "equip item") SEP
  // X(REMOVE, SDL_SCANCODE_R, "remove an item") SEP
  // X(TEXT, SDL_SCANCODE_TAB, "enter a command")

#define COMMA ,

#define ENUM_BUILDER(COMMAND, LETTER, DESC) COMMAND
#define INIT_BUILDER(COMMAND, LETTER, DESC) {LETTER, CommandType::COMMAND}
#define DESCRIPTION(COMMAND, LETTER, DESC) DESC
#define ENUM_NAME(COMMAND, LETTER, DESC) #COMMAND
