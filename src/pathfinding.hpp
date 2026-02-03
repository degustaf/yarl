#pragma once

#include <array>
#include <climits>
#include <cmath>
#include <memory>
#include <queue>
#include <vector>

#include <flecs.h>

#include "defines.hpp"
#include "game_map.hpp"

namespace pathfinding {

using Index = std::array<int, 2>;
inline bool operator==(const Index &lhs, const Index &rhs) {
  return lhs[0] == rhs[0] && lhs[1] == rhs[1];
}

template <typename T> class map {
public:
  map(const Index &dimensions) : map(dimensions, T()) {};
  map(const Index &dimensions, const T &val)
      : dimensions(dimensions),
        cost(std::make_unique<T[]>(dimensions[0] * dimensions[1])) {
    for (auto i = 0; i < dimensions[0] * dimensions[1]; i++) {
      cost[i] = val;
    }
  };

  inline T &operator[](const Index &idx) {
    return cost[idx[0] + dimensions[0] * idx[1]];
  };
  inline const T &operator[](const Index &idx) const {
    return cost[idx[0] + dimensions[0] * idx[1]];
  };
  inline void resize(size_t n) { cost.resize(n); }

  auto begin() { return cost.get(); };
  auto end() { return begin() + (size_t)(dimensions[0] * dimensions[1]); };

  Index dimensions;

  static map constructCost(flecs::entity mapEntity, T initial, T blocked,
                           T openable, T infinity) {
    return constructCost(mapEntity, mapEntity.get<GameMap>(), initial, blocked,
                         openable, infinity);
  }

  static map constructCost(flecs::entity mapEntity, const GameMap &gameMap,
                           T initial, T blocked, T openable, T infinity) {
    auto cost =
        map<int>(std::array{gameMap.getWidth(), gameMap.getHeight()}, initial);
    for (auto y = 0; y < gameMap.getHeight(); y++) {
      for (auto x = 0; x < gameMap.getWidth(); x++) {
        if (!gameMap.isWalkable({x, y})) {
          cost[{x, y}] = infinity;
        }
      }
    }

    auto q = mapEntity.world()
                 .query_builder<const Position>("module::blocks")
                 .with<BlocksMovement>()
                 .with(flecs::ChildOf, mapEntity)
                 .build();

    q.each([&](flecs::entity e, const Position &p) {
      if (e.has<Openable>()) {
        cost[p] = openable;
      } else {
        cost[p] = blocked;
      }
    });

    return cost;
  };

private:
  std::unique_ptr<T[]> cost;
};

inline std::vector<Index> constructPath(Index start, Index goal,
                                        const map<Index> &cameFrom) {
  auto path = std::vector<Index>{};
  auto current = goal;
  while (current != start) {
    path.push_back(current);
    current = cameFrom[current];
  }
  return path;
}

template <typename Heuristic>
std::vector<Index> aStar(const map<int> &cost, Index start, Index goal,
                         Heuristic h) {
  struct Node {
    Index idx;
    int cost;
  };
  auto comp = [](const Node &lhs, const Node rhs) {
    return lhs.cost > rhs.cost;
  };
  auto frontier =
      std::priority_queue<Node, std::vector<Node>, decltype(comp)>(comp);
  frontier.push(Node{start, h(start)});

  auto cameFrom = map<Index>(cost.dimensions);
  auto distance = map<int>(cost.dimensions, INT_MAX);
  distance[start] = 0;

  while (!frontier.empty()) {
    const auto current = frontier.top();
    frontier.pop();
    if (current.idx == goal) {
      return constructPath(start, goal, cameFrom);
    }

    for (const auto &dir : directions) {
      auto neighbor = Index{current.idx[0] + dir[0], current.idx[1] + dir[1]};
      if (cost[neighbor] == INT_MAX) {
        continue;
      }
      auto tentativeScore = distance[current.idx] + cost[neighbor];
      if (tentativeScore < distance[neighbor]) {
        cameFrom[neighbor] = current.idx;
        distance[neighbor] = tentativeScore;
        frontier.push(Node{neighbor, tentativeScore + h(neighbor)});
      }
    }
  }
  return std::vector<Index>();
}

class Dijkstra {
public:
  Dijkstra(int width, int height, map<int> &&cost)
      : width(width), height(height), distance({width, height}, infinity),
        cost(std::move(cost)), cameFrom({width, height}) {};
  inline void set(int x, int y, int value) { set({x, y}, value); };
  void set(Index xy, int value);
  void scan(void);
  std::array<int, 2> roll(int x, int y) const;
  Dijkstra &operator*=(float scale);
  std::array<int, 2> findMin(void) const;
  std::array<int, 2> findMax(void) const;
  std::vector<std::array<int, 2>> pathDown(std::array<int, 2> start) const;
  inline std::vector<std::array<int, 2>>
  pathUp(std::array<int, 2> start) const {
    return pathUp(start, findMax());
  }
  std::vector<std::array<int, 2>> pathUp(std::array<int, 2> start,
                                         std::array<int, 2> goal) const;

  static const int infinity;

private:
  int width;
  int height;
  map<int> distance;
  map<int> cost;
  map<Index> cameFrom;
};

class AutoExplore {
public:
  AutoExplore(flecs::entity map, const GameMap &gamemap)
      : gamemap(gamemap), cost({gamemap.getWidth(), gamemap.getHeight()}, 0),
        mapEntity(map) {};

  Index run(Index start);

private:
  static constexpr auto Infinity = 1 << 30;
  const GameMap &gamemap;
  map<int> cost;
  flecs::entity mapEntity;
};

} // namespace pathfinding
