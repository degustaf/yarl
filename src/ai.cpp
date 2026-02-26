#include "ai.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>

#include "action.hpp"
#include "actor.hpp"
#include "defines.hpp"
#include "fov.hpp"
#include "game_map.hpp"
#include "pathfinding.hpp"
#include "string.hpp"

std::unique_ptr<Action> HostileAi::act(flecs::entity self) {
  auto ecs = self.world();
  const auto &pos = self.get<Position>();
  auto player = ecs.lookup("player");
  auto inv = player.try_get_mut<Invisible>();
  const auto &target = player.get<Position>();
  const auto dx = target.x - pos.x;
  const auto dy = target.y - pos.y;
  const auto distance = std::max(std::abs(dx), std::abs(dy));
  auto mapEntity = ecs.lookup("currentMap").target<CurrentMap>();
  const auto &map = mapEntity.get<GameMap>();

  if (map.canSeePlayer(pos, target) && (!inv || inv->paused)) {
    if (distance <= 1) {
      return std::make_unique<MeleeAction>(dx, dy);
    }

    auto dij = pathfinding::Dijkstra(
        {map.getWidth(), map.getHeight()},
        [=](auto xy) { return target == xy; },
        [&](auto &xy) {
          auto ret = std::vector<pathfinding::Index>();
          ret.reserve(9); // 8 directions plus a portal
          for (auto &dir : directions) {
            auto next = pathfinding::Index{xy[0] + dir[0], xy[1] + dir[1]};
            if (map.inBounds(next) &&
                (map.isWalkable(next) ||
                 (self.has<Flying>() && map.isFlyable(next)))) {
              ret.push_back(next);
            }
          }
          flecs::entity e = ecs.query_builder<Position>()
                                .with(ecs.component<Portal>(), flecs::Wildcard)
                                .with(flecs::ChildOf, mapEntity)
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
                  .with(flecs::ChildOf, mapEntity)
                  .build()
                  .find([xy](auto &p) { return p == xy; })) {
            if (!map.isWalkable(xy)) {
              return 2;
            }
          }
          return 1;
        });
    dij.scan();
    path = pathfinding::constructPath(target, pos, dij.cameFrom);
    assert(pos == path[0]);
    std::reverse(path.begin(), path.end());
    path.pop_back();
  }

  if (path.size() > 0) {
    const auto [dest_x, dest_y] = path[path.size() - 1];
    path.pop_back();
    return std::make_unique<MoveAction>((int)dest_x - pos.x,
                                        (int)dest_y - pos.y);
  }

  return nullptr;
}

std::unique_ptr<Action> ConfusedAi::act(flecs::entity self) {
  if (turns_remaining <= 0) {
    self.remove<ConfusedAi>();

    auto ecs = self.world();
    auto ai = ecs.lookup("module::Ai");
    auto q = ecs.query_builder("module::ai").with(flecs::IsA, ai).build();
    q.each([self](auto ai) {
      if (self.has(ai) && !self.enabled(ai)) {
        self.enable(ai);
      }
    });

    auto msg = stringf("The %s is no longer confused.",
                       self.get<Named>().name.c_str());
    return std::make_unique<MessageAction>(msg);
  }

  auto rng = TCODRandom::getInstance();
  auto idx = rng->getInt(0, nDirections - 1);
  auto dxy = directions[idx];
  turns_remaining--;
  return std::make_unique<BumpAction>(dxy[0], dxy[1], 1);
}

std::unique_ptr<Action> FleeAi::act(flecs::entity self) {
  auto ecs = self.world();
  auto player = ecs.lookup("player");
  auto &playerPos = player.get<Position>();
  auto mapEntity = ecs.lookup("currentMap").target<CurrentMap>();
  const auto &map = mapEntity.get<GameMap>();

  // TODO handle invisibility
  auto dij = pathfinding::Dijkstra(
      {map.getWidth(), map.getHeight()},
      [=](auto xy) { return playerPos == xy; },
      [&](auto &xy) {
        auto ret = std::vector<pathfinding::Index>();
        ret.reserve(9); // 8 directions plus a portal
        for (auto &dir : directions) {
          auto next = pathfinding::Index{xy[0] + dir[0], xy[1] + dir[1]};
          if (map.inBounds(next) &&
              (map.isWalkable(next) ||
               (self.has<Flying>() && map.isFlyable(next)))) {
            ret.push_back(next);
          } else if (ecs.query_builder<const Position>()
                         .with<Openable>()
                         .with(flecs::ChildOf, mapEntity)
                         .build()
                         .find([next](auto &p) { return p == next; })) {
            ret.push_back(next);
          }
        }
        flecs::entity e = ecs.query_builder<Position>()
                              .with(ecs.component<Portal>(), flecs::Wildcard)
                              .with(flecs::ChildOf, mapEntity)
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
                .with(flecs::ChildOf, mapEntity)
                .build()
                .find([xy](auto &p) { return p == xy; })) {
          if (!map.isWalkable(xy)) {
            return 2;
          }
        }
        return 1;
      });
  dij.scan();
  dij *= -1.2f;
  dij.rescan();
  auto pos = self.get<Position>();
  auto xy = dij.cameFrom[pos];
  assert(xy[0] >= 0);
  assert(xy[1] >= 0);
  return std::make_unique<MoveAction>(xy[0] - pos.x, xy[1] - pos.y);
}

WanderAi::WanderAi(const GameMap &map)
    : Ai(), memory(map.getWidth() * map.getHeight(), pathfinding::Infinity) {
  for (auto y = 0; y < map.getHeight(); y++) {
    for (auto x = 0; x < map.getWidth(); x++) {
      if (map.isWalkable({x, y})) {
        memory[y * map.getWidth() + x] = 0;
      }
    }
  }
}

struct FovMap {
  FovMap(const GameMap &m)
      : width(m.getWidth()), height(m.getHeight()),
        tiles(std::make_unique<uint8_t[]>(width * height)) {
    for (auto y = 0; y < height; y++) {
      for (auto x = 0; x < width; x++) {
        auto &t = tiles[y * width + x];
        t = (m.isWalkable(x, y) ? WALKABLE : 0) |
            (m.isTransparent(x, y) ? TRANSPARENT : 0) |
            (m.isVisible(x, y) ? INFOV : 0);
      }
    }
  }

  inline int getWidth() const { return width; };
  inline int getHeight() const { return height; };
  inline bool inBounds(std::array<int, 2> xy) const {
    return 0 <= xy[0] && xy[0] < getWidth() && 0 <= xy[1] &&
           xy[1] < getHeight();
  }
  inline bool isWalkable(std::array<int, 2> xy) const {
    return inBounds(xy) && (tiles[xy[1] * getWidth() + xy[0]] & WALKABLE);
  }
  inline bool isTransparent(std::array<int, 2> xy) const {
    return inBounds(xy) && (tiles[xy[1] * getWidth() + xy[0]] & TRANSPARENT);
  }
  inline bool isFlyable(std::array<int, 2> xy) const {
    return isTransparent(xy) && !isWalkable(xy);
  };
  inline void setFov(std::array<int, 2> xy, bool seen) {
    assert(inBounds(xy));
    auto &t = tiles[xy[1] * getWidth() + xy[0]];
    if (seen) {
      t |= INFOV;
    } else {
      t &= ~INFOV;
    }
  }
  inline bool isVisible(std::array<int, 2> xy) const {
    return inBounds(xy) && (tiles[xy[1] * getWidth() + xy[0]] & INFOV);
  }

  int width;
  int height;
  std::unique_ptr<uint8_t[]> tiles;
  static constexpr auto WALKABLE = (uint8_t)1;
  static constexpr auto TRANSPARENT = (uint8_t)2;
  static constexpr auto INFOV = (uint8_t)4;
};

std::unique_ptr<Action> WanderAi::act(flecs::entity self) {
  for (auto &m : memory) {
    if (m != pathfinding::Infinity)
      m--;
  }
  auto ecs = self.world();
  auto mapEntity = ecs.lookup("currentMap").target<CurrentMap>();
  auto map = FovMap(mapEntity.get<GameMap>());
  computeFov(mapEntity, map, self.get<Position>(), 8);
  for (auto y = 0; y < map.getHeight(); y++) {
    for (auto x = 0; x < map.getWidth(); x++) {
      if (map.isVisible({x, y})) {
        memory[y * map.getWidth() + x] = 0;
      }
    }
  }

  auto dij = pathfinding::WanderDijkstra(
      {map.getWidth(), map.getHeight()}, memory,
      [&](auto &xy) {
        auto ret = std::vector<pathfinding::Index>();
        ret.reserve(9); // 8 directions plus a portal
        for (auto &dir : directions) {
          auto next = pathfinding::Index{xy[0] + dir[0], xy[1] + dir[1]};
          if (map.inBounds(next) &&
              (map.isWalkable(next) ||
               (self.has<Flying>() && map.isFlyable(next)))) {
            ret.push_back(next);
          } else if (ecs.query_builder<const Position>()
                         .with<Openable>()
                         .with(flecs::ChildOf, mapEntity)
                         .build()
                         .find([next](auto &p) { return p == next; })) {
            ret.push_back(next);
          }
        }
        flecs::entity e = ecs.query_builder<Position>()
                              .with(ecs.component<Portal>(), flecs::Wildcard)
                              .with(flecs::ChildOf, mapEntity)
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
                .with(flecs::ChildOf, mapEntity)
                .build()
                .find([xy](auto &p) { return p == xy; })) {
          if (!map.isWalkable(xy)) {
            return 2;
          }
        }
        return 1;
      });
  dij.scan();

  auto pos = self.get<Position>();
  auto xy = dij.cameFrom[pos];
  assert(xy[0] >= 0);
  assert(xy[1] >= 0);
  return std::make_unique<MoveAction>(xy[0] - pos.x, xy[1] - pos.y);
}
