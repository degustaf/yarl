#include "ai.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>

#include "action.hpp"
#include "actor.hpp"
#include "defines.hpp"
#include "game_map.hpp"
#include "pathfinding.hpp"
#include "string.hpp"

std::unique_ptr<Action> HostileAi::act(flecs::entity self) {
  auto ecs = self.world();
  const auto &pos = self.get<Position>();
  auto player = ecs.lookup("player");
  const auto &target = player.get<Position>();
  const auto dx = target.x - pos.x;
  const auto dy = target.y - pos.y;
  const auto distance = std::max(std::abs(dx), std::abs(dy));
  auto mapEntity = ecs.lookup("currentMap").target<CurrentMap>();
  const auto &map = mapEntity.get<GameMap>();

  if (map.isInFov(pos)) {
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
            if (map.inBounds(next) && map.isWalkable(next)) {
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

  auto dij = pathfinding::Dijkstra(
      {map.getWidth(), map.getHeight()},
      [=](auto xy) { return playerPos == xy; },
      [&](auto &xy) {
        auto ret = std::vector<pathfinding::Index>();
        ret.reserve(9); // 8 directions plus a portal
        for (auto &dir : directions) {
          auto next = pathfinding::Index{xy[0] + dir[0], xy[1] + dir[1]};
          if (map.inBounds(next) && map.isWalkable(next)) {
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
