#pragma once

#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <queue>
#include <vector>

#include "defines.hpp"

namespace pathfinding {

using Index = std::array<size_t, 2>;
inline bool operator==(const Index &lhs, const Index &rhs) {
  return lhs[0] == rhs[0] && lhs[1] == rhs[1];
}

template <typename T> class map {
public:
  map(const std::array<size_t, 2> &dimensions) : map(dimensions, T()){};
  map(const std::array<size_t, 2> &dimensions, const T &val)
      : dimensions(dimensions), cost(dimensions[0] * dimensions[1], val){};

  inline T &operator[](const Index &idx) {
    return cost[idx[0] + dimensions[0] * idx[1]];
  };
  inline const T &operator[](const Index &idx) const {
    return cost[idx[0] + dimensions[0] * idx[1]];
  };

  Index dimensions;

private:
  std::vector<T> cost;
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
std::vector<Index> aStar(map<int> cost, Index start, Index goal, Heuristic h) {
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

} // namespace pathfinding
