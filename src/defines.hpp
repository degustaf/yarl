#pragma once

constexpr int directions[][2] = {{1, 1},  {1, 0},  {1, -1}, {0, 1},
                                 {0, -1}, {-1, 1}, {-1, 0}, {-1, -1}};
constexpr auto nDirs = sizeof(directions) / sizeof(directions[0]);

constexpr auto saveFilename = "savegame.sav";
