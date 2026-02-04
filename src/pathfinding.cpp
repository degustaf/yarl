#include "pathfinding.hpp"
#include "actor.hpp"
#include "inventory.hpp"

using namespace pathfinding;

Index AutoExplore::run(Index start) {
  auto lambda = [&](const auto &lhs, const auto &rhs) {
    return cost[lhs] > cost[rhs];
  };
  std::priority_queue<Index, std::vector<Index>, decltype(lambda)> queue(
      lambda);

  for (auto y = 0; y < gamemap.getHeight(); y++) {
    for (auto x = 0; x < gamemap.getWidth(); x++) {
      if (gamemap.isExplored(x, y)) {
        cost[{x, y}] = Infinity;
      } else {
        cost[{x, y}] = 0;
        queue.push({x, y});
      }
    }
  }

  mapEntity.world()
      .query_builder<const Position>()
      .with<Item>()
      .with(flecs::ChildOf, mapEntity)
      .build()
      .each([&](const Position &p) {
        if (gamemap.isInFov(p)) {
          cost[p] = 0;
          queue.push(p);
        }
      });

  while (!queue.empty()) {
    auto next = queue.top();
    queue.pop();
    auto value = cost[next];
    for (auto &dir : directions) {
      auto x = next[0] + dir[0];
      auto y = next[1] + dir[1];
      if (!gamemap.inBounds({x, y})) {
        continue;
      }
      auto e = gamemap.get_blocking_entity(mapEntity, {x, y});
      if (e and e.has<Openable>()) {
        if (value + 2 < cost[{x, y}]) {
          cost[{x, y}] = value + 2;
          queue.push({x, y});
        }
      }
      if (!gamemap.isWalkable({x, y})) {
        continue;
      }
      if (value + 1 < cost[{x, y}]) {
        cost[{x, y}] = value + 1;
        queue.push({x, y});
      }
    }
  }

  auto ret = start;
  auto retCost = cost[start];
  for (auto &dir : directions) {
    auto xy = Index{start[0] + dir[0], start[1] + dir[1]};
    if (cost[xy] < retCost) {
      ret = xy;
      retCost = cost[xy];
    }
  }
  return ret;
};
