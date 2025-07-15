#include "room_accretion.hpp"
#include <array>

struct RectangularRoom {
  RectangularRoom(int x, int y, int width, int height)
      : x1(x), y1(y), x2(x + width), y2(y + height){};

  std::array<int, 2> center(void) const {
    return {(x1 + x2) / 2, (y1 + y2) / 2};
  };

  void carveOut(GameMap &map) const {
    for (auto y = y1 + 1; y < y2; y++) {
      for (auto x = x1 + 1; x < x2; x++) {
        map[{x, y}] = tile::floor_tile;
      }
    }
  }

  int x1;
  int y1;
  int x2;
  int y2;
};

GameMap generateDungeon(int width, int height) {
  auto dungeon = GameMap(width, height);

  auto room1 = RectangularRoom(20, 15, 10, 15);
  auto room2 = RectangularRoom(35, 15, 10, 15);

  room1.carveOut(dungeon);
  room2.carveOut(dungeon);

  return dungeon;
}
