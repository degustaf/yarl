#pragma once

#include <array>
#include <cctype>
#include <filesystem>
#include <string>

#include "string.hpp"

constexpr auto nDirections = 8;
constexpr int directions[nDirections][2] = {
    {1, 1}, {1, 0}, {1, -1}, {0, 1}, {0, -1}, {-1, 1}, {-1, 0}, {-1, -1}};

constexpr auto nFourDirections = 4;
constexpr int fourDirections[nFourDirections][2] = {
    {0, 1}, {0, -1}, {1, 0}, {-1, 0}};

inline std::string directionName(const int (&dir)[2]) {
  auto ret = stringf("%s%s",
                     dir[1]        ? ""
                     : dir[1] == 1 ? "south"
                                   : "north",
                     dir[0]        ? ""
                     : dir[0] == 1 ? "east"
                                   : "west");
  if (ret.size() > 0) {
    ret[0] = (char)toupper(ret[0]);
  }
  return ret;
}

inline std::string directionName(const std::array<int, 2> &dir) {
  return directionName({dir[0], dir[1]});
}

const std::filesystem::path data_dir = "save";
constexpr auto saveFilename = "savegame.sav";

constexpr auto DECORATION = std::array<int, 9>{
    0x250c, 0x2500, 0x2510, 0x2502, ' ', 0x2502, 0x2514, 0x2500, 0x2518};
