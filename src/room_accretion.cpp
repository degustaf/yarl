#include "room_accretion.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <queue>

#include <libtcod.hpp>

#include "actor.hpp"
#include "color.hpp"
#include "defines.hpp"
#include "engine.hpp"
#include "game_map.hpp"
#include "map_shared.hpp"
#include "pathfinding.hpp"
#include "scent.hpp"

using namespace roomAccretion;

static constexpr auto MAX_DUNGEON_LEVEL = 10;

static void dig(int x1, int y1, int x2, int y2, GameMap &map) {
  if (x2 < x1) {
    auto tmp = x1;
    x1 = x2;
    x2 = tmp;
  }

  if (y2 < y1) {
    auto tmp = y1;
    y1 = y2;
    y2 = tmp;
  }

  for (auto tilex = x1; tilex <= x2; tilex++) {
    for (auto tiley = y1; tiley <= y2; tiley++) {
      map.carveOut(tilex, tiley);
    }
  }
}

static RectangularRoom firstRoom(const Config &cfg, int width, int height,
                                 GameMap &map, TCODRandom &rng) {
  while (true) {
    auto x = rng.getInt(0, width - 1);
    auto y = rng.getInt(0, height - 1);
    auto w = rng.getInt(cfg.ROOM_MIN_SIZE, cfg.ROOM_MAX_SIZE);
    auto h = rng.getInt(cfg.ROOM_MIN_SIZE, cfg.ROOM_MAX_SIZE);

    if (x + w >= width - 1 || y + h >= height - 1) {
      continue;
    }

    auto ret = RectangularRoom{x, y, w, h};
    ret.carveOut(map);

    return ret;
  }
}

static bool canDig(int width, int height, int x1, int y1, int x2, int y2,
                   const GameMap &map) {
  if (x2 < x1) {
    auto tmp = x1;
    x1 = x2;
    x2 = tmp;
  }

  if (y2 < y1) {
    auto tmp = y1;
    y1 = y2;
    y2 = tmp;
  }

  if (x1 < 1 || y1 < 1 || x2 >= width - 1 || y2 >= height - 1) {
    return false;
  }

  for (auto tilex = x1 - 1; tilex <= x2 + 1; tilex++) {
    for (auto tiley = y1 - 1; tiley <= y2 + 1; tiley++) {
      if (map.isWalkable(tilex, tiley)) {
        return false;
      }
    }
  }

  return true;
}

std::optional<RectangularRoom> addRoom(const Config &cfg, int width, int height,
                                       GameMap &map,
                                       std::vector<std::array<int, 2>> &doors,
                                       TCODRandom &rng) {
  auto idx = rng.getInt(0, nFourDirections - 1);
  auto &dir = fourDirections[idx];
  auto edges = std::vector<std::array<int, 2>>();
  edges.reserve(height * width);

  for (auto x = 0; x < width; x++) {
    for (auto y = 0; y < height; y++) {
      if (!map.isWalkable(x, y)) {
        if (map.isWalkable(x - dir[0], y - dir[1])) {
          if (!map.isWalkable(x + dir[0], y + dir[1])) {
            edges.push_back({x, y});
          }
        }
      }
    }
  }

  auto edge = edges[rng.getInt(0, (int)edges.size() - 1)];
  auto corridor_length = 1;

  if (rng.getDouble(0, 1.0) <= cfg.CORRIDOR_PERCENT) {
    corridor_length = rng.get(cfg.ROOM_MIN_SIZE, cfg.ROOM_MAX_SIZE);
    for (auto i = 1; i < corridor_length; i++) {
      if (map.isWalkable(edge[0] + i * dir[0], edge[1] + i * dir[1])) {
        return std::nullopt;
      }
    }
  }

  auto w = rng.getInt(cfg.ROOM_MIN_SIZE, cfg.ROOM_MAX_SIZE);
  auto h = rng.getInt(cfg.ROOM_MIN_SIZE, cfg.ROOM_MAX_SIZE);
  auto x1 = 0, y1 = 0, x2 = 0, y2 = 0;
  if (dir[0] == 0 && dir[1] == 1) {
    x1 = edge[0] - w / 2;
    y1 = edge[1] + corridor_length;
    x2 = x1 + w;
    y2 = y1 + h;
  } else if (dir[0] == 0 && dir[1] == -1) {
    x1 = edge[0] - w / 2;
    y1 = edge[1] - corridor_length;
    x2 = x1 + w;
    y2 = y1 - h;
  } else if (dir[0] == 1 && dir[1] == 0) {
    x1 = edge[0] + corridor_length;
    y1 = edge[1] - h / 2;
    x2 = x1 + w;
    y2 = y1 + h;
  } else if (dir[0] == -1 && dir[1] == 0) {
    x1 = edge[0] - corridor_length;
    y1 = edge[1] - h / 2;
    x2 = x1 - w;
    y2 = y1 + h;
  }

  if (!canDig(width, height, x1, y1, x2, y2, map)) {
    return std::nullopt;
  }

  doors.push_back(edge);

  dig(edge[0], edge[1], edge[0] + corridor_length * dir[0],
      edge[1] + corridor_length * dir[1], map);
  dig(x1, y1, x2, y2, map);
  if (x1 > x2) {
    auto temp = x1;
    x1 = x2;
    x2 = temp;
  }
  if (y1 > y2) {
    auto temp = y1;
    y1 = y2;
    y2 = temp;
  }
  return RectangularRoom{x1, y1, x2 - x1, y2 - y1};
}

static void addRooms(const Config &cfg, int width, int height, GameMap &map,
                     std::vector<RectangularRoom> &rooms,
                     std::vector<std::array<int, 2>> &doors, TCODRandom &rng) {
  for (auto i = 0; i < cfg.MAX_ITER; i++) {
    if (auto rm = addRoom(cfg, width, height, map, doors, rng)) {
      rooms.push_back(*rm);
    }

    if (rooms.size() == cfg.MAX_ROOMS) {
      return;
    }
  }
}

static void addLoops(const Config &cfg, int width, int height, GameMap &map,
                     TCODRandom &rng, int length) {
  static const int dirs[2][2] = {{1, 0}, {0, 1}};
  std::vector<std::array<int, 3>> thinWalls;

  for (auto y = 1; y < height - 1; y++) {
    for (auto x = 1; x < width - 1; x++) {
      for (auto idx = 0; idx < 2; idx++) {
        if (!map.isWalkable(x, y) &&
            map.isWalkable(x + length * dirs[idx][0],
                           y + length * dirs[idx][1]) &&
            map.isWalkable(x - dirs[idx][0], y - dirs[idx][1])) {
          thinWalls.push_back({x, y, idx});
        }
      }
    }
  }

  while (!thinWalls.empty()) {
    auto idx = rng.getInt(0, (int)thinWalls.size() - 1);
    auto &[x, y, dxy] = thinWalls[idx];
    auto &[dx, dy] = dirs[dxy];
    auto x1 = x + length * dx;
    auto y1 = y + length * dy;
    auto x2 = x - dx;
    auto y2 = y - dy;

    bool addLoop = true;
    for (auto i = 0; i < length; i++) {
      if (map.isWalkable(x + i * dx, y + i * dy)) {
        addLoop = false;
      }
    }

    if (addLoop) {
      auto dij = pathfinding::Dijkstra(
          {map.getWidth(), map.getHeight()},
          [=](auto xy) { return xy[0] == x1 && xy[1] == y1; },
          [&](auto &xy) {
            auto ret = std::vector<pathfinding::Index>();
            ret.reserve(8);
            for (auto &dir : directions) {
              auto next = pathfinding::Index{xy[0] + dir[0], xy[1] + dir[1]};
              if (map.inBounds(next) && map.isWalkable(next)) {
                ret.push_back(next);
              }
            }
            return ret;
          },
          [&](auto) { return 1; });
      dij.scan();

      auto path = pathfinding::constructPath({x1, y1}, {x2, y2}, dij.cameFrom);
      if (path.size() > cfg.MIN_LOOP_DISTANCE) {
        for (auto i = 0; i < length; i++) {
          map.carveOut(x + i * dx, y + i * dy);
        }
      }
    }
    thinWalls.erase(thinWalls.begin() + idx);
  }
}

struct Lake {
  std::vector<std::array<int, 2>> tiles;
  int minx;
  int miny;
  int maxx;
  int maxy;
};

static Lake floodFill(std::vector<bool> &area, int width, int height, int x,
                      int y) {
  std::queue<std::array<int, 2>> q;
  q.push({x, y});
  Lake ret{{}, x, y, x, y};

  while (!q.empty()) {
    auto next = q.front();
    q.pop();
    for (auto dir : fourDirections) {
      auto x1 = next[0] + dir[0];
      auto y1 = next[1] + dir[1];
      if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height &&
          area[y1 * width + x1]) {
        q.push({x1, y1});
        area[y1 * width + x1] = false;
        ret.tiles.push_back({x1, y1});
        if (x1 < ret.minx) {
          ret.minx = x1;
        }
        if (x1 > ret.maxx) {
          ret.maxx = x1;
        }
        if (y1 < ret.miny) {
          ret.miny = y1;
        }
        if (y1 > ret.maxy) {
          ret.maxy = y1;
        }
      }
    }
  }

  return ret;
}

static void floodFill(TCODMap &m, int x, int y) {
  std::queue<std::array<int, 2>> q;
  q.push({x, y});

  while (!q.empty()) {
    auto next = q.front();
    q.pop();
    for (auto dir : fourDirections) {
      auto x1 = next[0] + dir[0];
      auto y1 = next[1] + dir[1];
      if (m.isWalkable(x1, y1)) {
        q.push({x1, y1});
        m.setProperties(x1, y1, false, false);
      }
    }
  }
}

static void addLake(const Config &cfg, int width, int height, TCODRandom &rng,
                    GameMap &map, bool water) {
  auto area = std::vector<bool>(width * height, false);
  for (auto i = 0; i < width * height; i++) {
    area[i] = rng.getDouble(0.0, 1.0) < cfg.CELLULAR_AUTOMATA_PERCENT;
  }

  for (auto i = 0; i < cfg.CELLULAR_AUTOMATA_ITER; i++) {
    auto count = std::vector<int>(width * height, 0);
    for (auto y = 0; y < height; y++) {
      for (auto x = 0; x < width; x++) {
        for (auto &adj : directions) {
          if (0 <= x + adj[0] && x + adj[0] < width && 0 <= y + adj[1] &&
              y + adj[1] < height &&
              area[(y + adj[1]) * width + (x + adj[0])]) {
            count[y * width + x]++;
          }
        }
      }
    }

    for (auto j = 0; j < width * height; j++) {
      if (area[j]) {
        area[j] = count[j] > cfg.STAY_WALL;
      } else {
        area[j] = count[j] > cfg.BECOME_WALL;
      }
    }
  }

  Lake best;
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < width; x++) {
      if (area[y * width + x]) {
        auto l = floodFill(area, width, height, x, y);
        if (l.maxx - l.minx + 1 >= cfg.MIN_LAKE_DIMENSION &&
            l.maxy - l.miny + 1 >= cfg.MIN_LAKE_DIMENSION &&
            l.maxx - l.minx + 1 <= cfg.MAX_LAKE_WIDTH &&
            l.maxy - l.miny + 1 <= cfg.MAX_LAKE_HEIGHT &&
            l.tiles.size() > best.tiles.size()) {
          best = l;
        }
      }
    }
  }
  if (best.tiles.empty()) {
    // No appropriately sized lake.
    return;
  }

  if (cfg.FORCE_CONNECTED) {
    TCODMap m(width, height);
    m.copy(&map.get());
    for (auto &xy : best.tiles) {
      m.setProperties(xy[0], xy[1], true, false);
    }
    auto found = false;
    for (auto y = 0; y < height; y++) {
      for (auto x = 0; x < width; x++) {
        if (m.isWalkable(x, y)) {
          if (found) {
            // Lake disconnects the map. reject it;
            return;
          } else {
            floodFill(m, x, y);
            found = true;
          }
        }
      }
    }
  }

  for (auto &xy : best.tiles) {
    map.setProperties(xy[0], xy[1], true, false);
  }

  if (water) {
    for (auto &xy : best.tiles) {
      map.tiles[xy[1] * width + xy[0]].flags = Tile::Water;
    }
  }
}

static std::array<int, 2> generateStairs(std::vector<RectangularRoom> &rooms,
                                         GameMap &map) {
  for (auto i = rooms.rbegin(); i != rooms.rend(); i++) {
    auto &rm = *i;

    auto [x, y] = rm.center();
    assert(map.isTransparent(x, y));
    if (map.isWalkable(x, y)) {
      // This prevents us from placing stairs in a lake.
      if (map.level < MAX_DUNGEON_LEVEL) {
        map.makeStairs(x, y);
      }
      return {x, y};
    }
  }
  assert(false);
  return {0, 0};
}

static constexpr auto item_weights = std::array<WeightsByFloor, 11>{
    WeightsByFloor{1, 350, "module::lightningScroll"},
    WeightsByFloor{1, 35, "module::healthPotion"},
    WeightsByFloor{1, 35, "module::deodorant"},
    WeightsByFloor{1, 35, "module::dung"},
    WeightsByFloor{2, 35, "module::rope"},
    WeightsByFloor{2, 35, "module::taser"},
    WeightsByFloor{3, 35, "module::scanner"},
    WeightsByFloor{3, 35, "module::transporter"},
    WeightsByFloor{3, 35, "module::mapper"},
    WeightsByFloor{3, 35, "module::tracker"},
    WeightsByFloor{4, 30, "module::45"}};

static constexpr auto monster_weights =
    std::array<WeightsByFloor, 2>{WeightsByFloor{1, 20, "module::orc"},
                                  WeightsByFloor{4, 20, "module::cysts"}};

static void populateRoom(const Config &cfg, flecs::entity map,
                         flecs::entity player, TCODRandom &rng, bool &first,
                         const GameMap &dungeon, const RectangularRoom &room,
                         flecs::query<const Position> q) {
  auto ecs = map.world();
  if (first && dungeon.isWalkable(room.center())) {
    player.get_mut<Position>() = room.center();
    if (!cfg.lit) {
      auto x = rng.getInt(room.x1 + 1, room.x2 - 1);
      auto y = rng.getInt(room.y1 + 1, room.y2 - 1);
      ecs.entity()
          .is_a(ecs.lookup("module::light"))
          .set<Position>({x, y})
          .add(flecs::ChildOf, map);
    }
    if (dungeon.level == 1) {
      while (true) {
        auto x = rng.getInt(room.x1 + 1, room.x2 - 1);
        auto y = rng.getInt(room.y1 + 1, room.y2 - 1);
        if (dungeon.isWalkable(x, y) && q.find([x, y](const Position &p) {
              return p == Position{x, y};
            }) == map.null()) {
          auto prefab = ecs.lookup("module::lightScroll");
          assert(prefab);
          ecs.entity().is_a(prefab).set<Position>({x, y}).add(flecs::ChildOf,
                                                              map);
          break;
        }
      }
    }
    first = false;
  } else {
    const auto item_count =
        rng.getInt(0, getMaxValueForFloor(max_items_by_floor, dungeon.level));
    for (auto i = 0; i < item_count; i++) {
      auto x = rng.getInt(room.x1 + 1, room.x2 - 1);
      auto y = rng.getInt(room.y1 + 1, room.y2 - 1);
      if (dungeon.isWalkable(x, y) && q.find([x, y](const Position &p) {
            return p == Position{x, y};
          }) == map.null()) {
        auto prefab =
            ecs.lookup(get_entity_at_random(item_weights, dungeon.level, rng));
        assert(prefab);
        ecs.entity().is_a(prefab).set<Position>({x, y}).add(flecs::ChildOf,
                                                            map);
      }
    }

    const auto monster_count = rng.getInt(
        0, getMaxValueForFloor(max_monsters_by_floor, dungeon.level));
    for (auto i = 0; i < monster_count; i++) {
      auto x = rng.getInt(room.x1 + 1, room.x2 - 1);
      auto y = rng.getInt(room.y1 + 1, room.y2 - 1);
      if (dungeon.isWalkable(x, y) && q.find([x, y](const Position &p) {
            return p == Position{x, y};
          }) == map.null()) {
        auto prefab = ecs.lookup(
            get_entity_at_random(monster_weights, dungeon.level, rng));
        assert(prefab);
        ecs.entity().is_a(prefab).set<Position>({x, y}).add(flecs::ChildOf,
                                                            map);
      }
    }

    if (!cfg.lit) {
      if (rng.get(0.0, 1.0) < cfg.LIGHT_PERCENT) {
        auto x = rng.getInt(room.x1 + 1, room.x2 - 1);
        auto y = rng.getInt(room.y1 + 1, room.y2 - 1);
        ecs.entity()
            .is_a(ecs.lookup("module::light"))
            .set<Position>({x, y})
            .add(flecs::ChildOf, map);
      }
    }
  }
}

GameMap roomAccretion::generateDungeon(const Config &cfg, flecs::entity map,
                                       int width, int height, int level,
                                       flecs::entity player) {
  auto dungeon = GameMap(width, height, level, cfg.lit);
  generateDungeon(cfg, map, dungeon, player, true);
  return dungeon;
}

void decreaseSmeller(flecs::entity e) {
  e.get_mut<Smeller>().threshold -= 10.0f;
}

void decreaseScent(flecs::entity e) { e.get_mut<Scent>().power -= 10.0f; }

template <typename _RandomAccessIterator, typename _RandomNumberGenerator>
void random_shuffle(_RandomAccessIterator __first, _RandomAccessIterator __last,
                    _RandomNumberGenerator &&__rand) {
  if (__first == __last)
    return;
  for (auto __i = __first + 1; __i != __last; ++__i) {
    auto __j = __first + __rand((__i - __first) + 1);
    if (__i != __j)
      std::iter_swap(__i, __j);
  }
}

static void addPortals(const Config &cfg, flecs::entity map, GameMap &dungeon,
                       TCODRandom &rng) {
  auto width = dungeon.getWidth();
  auto height = dungeon.getHeight();
  for (auto i = 0; i < cfg.PORTALS; i++) {
    auto e1 = map.world()
                  .entity()
                  .add(flecs::ChildOf, map)
                  .add<BlocksFov>()
                  .set<Renderable>({'A', color::portal, std::nullopt,
                                    RenderOrder::Corpse, false});
    auto e2 = map.world()
                  .entity()
                  .add(flecs::ChildOf, map)
                  .add<Portal>(e1)
                  .add<BlocksFov>()
                  .set<Renderable>({'A', color::portal, std::nullopt,
                                    RenderOrder::Corpse, false});
    while (true) {
      auto x = rng.getInt(0, width - 1);
      auto y = rng.getInt(0, height - 1);
      if (dungeon.isWalkable({x, y})) {
        assert(dungeon.isTransparent({x, y}));
        e1.set<Position>({x, y});
        break;
      }
    }

    while (true) {
      auto x = rng.getInt(0, width - 1);
      auto y = rng.getInt(0, height - 1);
      if (e1.get<Position>() == Position{x, y})
        continue;
      if (dungeon.isWalkable({x, y})) {
        assert(dungeon.isTransparent({x, y}));
        e2.set<Position>({x, y});
        break;
      }
    }
  }
}

void roomAccretion::generateDungeon(const Config &cfg, flecs::entity map,
                                    GameMap &dungeon, flecs::entity player,
                                    bool generateEntities) {
  auto ecs = map.world();
  auto seed = ecs.lookup("seed").get<Seed>();
  auto rng = TCODRandom(seed.seed + dungeon.level);
  auto width = dungeon.getWidth();
  auto height = dungeon.getHeight();

  auto rooms = std::vector<RectangularRoom>();
  rooms.push_back(firstRoom(cfg, width, height, dungeon, rng));

  auto doors = std::vector<std::array<int, 2>>();
  addRooms(cfg, width, height, dungeon, rooms, doors, rng);
  for (auto i = 1; i <= cfg.LOOP_ITER; i++) {
    addLoops(cfg, width, height, dungeon, rng, i);
  }
  if (dungeon.level < MAX_DUNGEON_LEVEL) {
    for (auto i = 0; i < cfg.LAKE_ITER; i++) {
      addLake(cfg, width, height, rng, dungeon, rng.get(0, 1) == 0);
    }
  }
  auto stairs = generateStairs(rooms, dungeon);

  if (!generateEntities) {
    return;
  }

  auto q = ecs.query_builder<const Position>("module::position")
               .with(flecs::ChildOf, map)
               .build();

  addPortals(cfg, map, dungeon, rng);

  if (dungeon.level == MAX_DUNGEON_LEVEL) {
    auto yendor = ecs.lookup("module::yendor");
    assert(yendor);
    ecs.entity().is_a(yendor).set<Position>(stairs).add(flecs::ChildOf, map);
  }

  auto firstRoom = true;
  for (auto &rm : rooms) {
    populateRoom(cfg, map, player, rng, firstRoom, dungeon, rm, q);
  }

  for (auto i = rooms.size() - 1; i > 0; i--) {
    // i > 0 is intentional. We don't want a fountain in the first roomwith the
    // player.
    auto center = rooms[i].center();
    if (dungeon.isWalkable(center) && !dungeon.isStairs(center) &&
        rng.get(0.0, 0.1) < cfg.FOUNTAIN_PERCENT) {
      ecs.entity()
          .set<Position>(center)
          .is_a(ecs.lookup("module::fountain"))
          .add(flecs::ChildOf, map);
    }
  }

  for (auto d : doors) {
    if (rng.getDouble(0.0, 1.0) < cfg.DOOR_PERCENTAGE) {
      if (dungeon.isWalkable(d) && dungeon.isTransparent(d)) {
        auto e = ecs.entity()
                     .is_a(ecs.lookup("module::door"))
                     .set<Position>(d)
                     .add<BlocksMovement>()
                     .add<BlocksFov>()
                     .add(flecs::ChildOf, map);
        dungeon.setProperties(d[0], d[1], !e.has<BlocksFov>(), false);
      }
    }
  }

  auto pos = player.get<Position>();
  auto dij = pathfinding::Dijkstra(
      {dungeon.getWidth(), dungeon.getHeight()},
      [=](auto xy) { return pos == xy || stairs == xy; },
      [&](auto &xy) {
        auto ret = std::vector<pathfinding::Index>();
        ret.reserve(9); // 8 directions plus a portal
        for (auto &dir : directions) {
          auto next = pathfinding::Index{xy[0] + dir[0], xy[1] + dir[1]};
          if (dungeon.inBounds(next) && dungeon.isWalkable(next)) {
            ret.push_back(next);
          }
        }
        flecs::entity e = ecs.query_builder<Position>()
                              .with(ecs.component<Portal>(), flecs::Wildcard)
                              .with(flecs::ChildOf, map)
                              .build()
                              .find([xy](auto &p) { return p == xy; });
        if (e) {
          ret.push_back(e.target<Portal>().get<Position>());
        }
        return ret;
      },
      [&](auto xy) {
        if (ecs.query_builder<const Position>()
                .with<Openable>()
                .with(flecs::ChildOf, map)
                .build()
                .find([xy](auto &p) { return p == xy; })) {
          if (!dungeon.isWalkable(xy)) {
            return 2;
          }
        }
        return 1;
      });

  dij.scan();
}
