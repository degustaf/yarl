#include "room_accretion.hpp"

#include <libtcod.hpp>

#include <array>

#include "actor.hpp"

struct RectangularRoom {
  RectangularRoom(int x, int y, int width, int height)
      : x1(x), y1(y), x2(x + width), y2(y + height){};
  RectangularRoom() : x1(0), y1(0), x2(0), y2(0){};

  std::array<int, 2> center(void) const {
    return {(x1 + x2) / 2, (y1 + y2) / 2};
  };

  void carveOut(GameMap &map) const {
    for (auto y = y1 + 1; y < y2; y++) {
      for (auto x = x1 + 1; x < x2; x++) {
        map.carveOut(x, y);
      }
    }
  }

  inline bool intersect(const RectangularRoom &other) const {
    return x1 <= other.x2 && x2 >= other.x1 && y1 <= other.y2 && y2 >= other.y1;
  };

  int x1;
  int y1;
  int x2;
  int y2;
};

static void tunnel_between(GameMap &map, const std::array<int, 2> &start,
                           const std::array<int, 2> &end) {
  auto center = [&] {
    auto rng = TCODRandom();
    if (rng.getInt(0, 1) == 0) {
      return std::array<int, 2>{start[0], end[1]};
    } else {
      return std::array<int, 2>{end[0], start[1]};
    }
  }();

  for (const auto &xy : tcod::BresenhamLine(start, center)) {
    map.carveOut(xy[0], xy[1]);
  }
  for (const auto &xy : tcod::BresenhamLine(center, end)) {
    map.carveOut(xy[0], xy[1]);
  }
}

static const auto ROOM_MAX_SIZE = 10;
static const auto ROOM_MIN_SIZE = 6;
static const auto MAX_ROOMS = 30;

GameMap generateDungeon(int width, int height, flecs::entity player) {
  auto dungeon = GameMap(width, height);

  auto rooms = std::array<RectangularRoom, MAX_ROOMS>();
  size_t roomCount = 0;
  auto rng = TCODRandom();

  for (size_t i = 0; i < MAX_ROOMS; i++) {
    auto room_width = rng.getInt(ROOM_MIN_SIZE, ROOM_MAX_SIZE);
    auto room_height = rng.getInt(ROOM_MIN_SIZE, ROOM_MAX_SIZE);
    auto x = rng.getInt(0, width - room_width - 1);
    auto y = rng.getInt(0, height - room_height - 1);

    auto new_room = RectangularRoom(x, y, room_width, room_height);
    auto intersects = false;
    for (size_t j = 0; j < i; j++) {
      if (new_room.intersect(rooms[j])) {
        intersects = true;
        break;
      }
    }
    if (intersects) {
      continue;
    }

    new_room.carveOut(dungeon);
    if (roomCount == 0) {
      player.get_mut<Position>() = new_room.center();
    } else {
      tunnel_between(dungeon, rooms[roomCount - 1].center(), new_room.center());
    }
    rooms[roomCount] = new_room;
    roomCount++;
  }

  return dungeon;
}
