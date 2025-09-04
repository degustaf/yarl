#include "room_accretion.hpp"

#include <libtcod.hpp>

#include <array>
#include <cassert>
#include <cstddef>

#include "actor.hpp"
#include "engine.hpp"

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
                           const std::array<int, 2> &end, TCODRandom &rng) {
  auto center = [&] {
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

static constexpr auto ROOM_MAX_SIZE = 10;
static constexpr auto ROOM_MIN_SIZE = 6;
static constexpr auto MAX_ROOMS = 30;

struct MaxByFloor {
  int minFloor;
  int count;
};

static constexpr auto max_items_by_floor =
    std::array<MaxByFloor, 2>{MaxByFloor{1, 1}, {4, 2}};

static constexpr auto max_monsters_by_floor =
    std::array<MaxByFloor, 3>{MaxByFloor{1, 2}, {4, 3}, {6, 5}};

template <size_t N>
static int getMaxValueForFloor(const std::array<MaxByFloor, N> &maxByFloor,
                               int floor) {
  auto currentCount = 0;
  for (const auto &f : maxByFloor) {
    if (f.minFloor > floor) {
      break;
    } else {
      currentCount = f.count;
    }
  }
  return currentCount;
}

struct WeightsByFloor {
  int minFloor;
  int weight;
  const char *name;
};

static constexpr auto item_weights = std::array<WeightsByFloor, 6>{
    WeightsByFloor{0, 35, "module::healthPotion"},
    {2, 10, "module::confusionScroll"},
    {4, 25, "module::lightningScroll"},
    {4, 5, "module::sword"},
    {6, 25, "module::fireballScroll"},
    {6, 15, "module::chainMail"},
};

static constexpr auto enemy_weights =
    std::array<WeightsByFloor, 4>{WeightsByFloor{0, 80, "module::orc"},
                                  {3, 15, "module::troll"},
                                  {5, 30, "module::troll"},
                                  {7, 60, "module::troll"}};

template <size_t N>
static const char *
get_entity_at_random(const std::array<WeightsByFloor, N> &weights, int floor,
                     TCODRandom &rng) {
  auto totalWeight = 0;
  for (const auto &w : weights) {
    if (w.minFloor > floor) {
      break;
    }
    totalWeight += w.weight;
  }

  auto choice = rng.getInt(1, totalWeight);
  for (const auto &w : weights) {
    if (choice <= w.weight) {
      return w.name;
    }
    choice -= w.weight;
  }
  assert(false);
  return "";
}

static void place_entities(flecs::entity map, const RectangularRoom &r,
                           int level, TCODRandom &rng) {
  const auto monster_count =
      rng.getInt(0, getMaxValueForFloor(max_monsters_by_floor, level));
  const auto item_count =
      rng.getInt(0, getMaxValueForFloor(max_items_by_floor, level));

  auto ecs = map.world();
  auto q = ecs.query_builder<const Position>("module::position")
               .with(flecs::ChildOf, map)
               .build();

  for (auto i = 0; i < monster_count; i++) {
    auto x = rng.getInt(r.x1 + 1, r.x2 - 1);
    auto y = rng.getInt(r.y1 + 1, r.y2 - 1);
    auto pos = Position(x, y);

    auto e = q.find([&](const auto &p) { return p == pos; });
    if (e == e.null()) {
      auto prefab = ecs.lookup(get_entity_at_random(enemy_weights, level, rng));
      assert(prefab);
      ecs.entity().is_a(prefab).set<Position>(pos).add(flecs::ChildOf, map);
    }
  }

  for (auto i = 0; i < item_count; i++) {
    auto x = rng.getInt(r.x1 + 1, r.x2 - 1);
    auto y = rng.getInt(r.y1 + 1, r.y2 - 1);
    auto pos = Position(x, y);

    auto e = q.find([&](const auto &p) { return p == pos; });
    if (e == e.null()) {
      auto prefab = ecs.lookup(get_entity_at_random(item_weights, level, rng));
      assert(prefab);
      ecs.entity().is_a(prefab).set<Position>(pos).add(flecs::ChildOf, map);
    }
  }
}

GameMap generateDungeon(flecs::entity map, int width, int height, int level,
                        flecs::entity player) {
  auto dungeon = GameMap(width, height, level);
  generateDungeon(map, dungeon, player, true);
  return dungeon;
}

void generateDungeon(flecs::entity map, GameMap &dungeon, flecs::entity player,
                     bool generateEntities) {
  auto rooms = std::array<RectangularRoom, MAX_ROOMS>();
  size_t roomCount = 0;
  auto seed = map.world().lookup("seed").get<Seed>();
  auto rng = TCODRandom(seed.seed + dungeon.level);
  auto width = dungeon.getWidth();
  auto height = dungeon.getHeight();

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
      if (generateEntities) {
        player.get_mut<Position>() = new_room.center();
      }
    } else {
      tunnel_between(dungeon, rooms[roomCount - 1].center(), new_room.center(),
                     rng);
    }

    rooms[roomCount] = new_room;
    roomCount++;
  }

  auto downStairs = rooms[roomCount - 1].center();
  dungeon.tiles[(size_t)(downStairs[1] * width + downStairs[0])].flags |=
      Tile::Stairs;

  if (generateEntities) {
    for (size_t i = 1; i < roomCount; i++) {
      place_entities(map, rooms[i], dungeon.level, rng);
    }
  }
}
