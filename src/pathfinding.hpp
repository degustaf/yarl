#pragma once

#include <array>
#include <climits>
#include <cmath>
#include <limits>
#include <memory>
#include <queue>
#include <vector>

#include <flecs.h>

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
    assert(idx[0] >= 0);
    assert(idx[1] >= 0);
    return cost[idx[0] + dimensions[0] * idx[1]];
  };
  inline const T &operator[](const Index &idx) const {
    assert(idx[0] >= 0);
    assert(idx[1] >= 0);
    return cost[idx[0] + dimensions[0] * idx[1]];
  };
  inline void resize(size_t n) { cost.resize(n); }

  auto begin() { return cost.get(); };
  auto end() { return begin() + (size_t)(dimensions[0] * dimensions[1]); };

  Index dimensions;

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

static constexpr auto Infinity = std::numeric_limits<int>::max();

/******
 * F: std::function<bool(Index)>
 * G: std::function<RangedFor<Index>(Index)>
 * H: std::function<int(Index)>
 * *****/
template <typename F, typename G, typename H> class Dijkstra {
public:
  Dijkstra(Index dimensions, F start, G adjacent, H cost)
      : dimensions(dimensions), distance(dimensions, Infinity),
        cameFrom(dimensions, {-1, -1}), start(start), adjacent(adjacent),
        cost(cost) {};

  Dijkstra &operator*=(float f) {
    for (auto &d : distance) {
      if (d != Infinity) {
        d = (int)((float)d * f);
      }
    }
    return *this;
  }

  void scan(void) {
    auto order = [&](const auto &lhs, const auto &rhs) {
      return distance[lhs] > distance[rhs];
    };
    std::priority_queue<Index, std::vector<Index>, decltype(order)> queue(
        order);
    for (auto y = 0; y < dimensions[1]; y++) {
      for (auto x = 0; x < dimensions[0]; x++) {
        if (start(Index{x, y})) {
          distance[{x, y}] = 0;
          queue.push({x, y});
        } else {
          distance[{x, y}] = Infinity;
        }
      }
    }

    scanPrivate(queue);
  }

  void rescan(void) {
    auto order = [&](const auto &lhs, const auto &rhs) {
      return distance[lhs] > distance[rhs];
    };
    std::priority_queue<Index, std::vector<Index>, decltype(order)> queue(
        order);
    for (auto y = 0; y < dimensions[1]; y++) {
      for (auto x = 0; x < dimensions[0]; x++) {
        if (start(Index{x, y})) {
          queue.push({x, y});
        }
      }
    }

    scanPrivate(queue);
  }

private:
  template <typename Q> inline void scanPrivate(Q &queue) {
    while (!queue.empty()) {
      auto next = queue.top();
      queue.pop();
      auto value = distance[next];
      if (value == Infinity) {
        return;
      }

      for (auto &v : adjacent(next)) {
        auto alt = value + cost(v);
        if (alt < distance[v]) {
          distance[v] = alt;
          queue.push(v);
          cameFrom[v] = next;
        }
      }
    }
  }

  Index dimensions;
  map<int> distance;

public:
  map<Index> cameFrom;

private:
  F start;
  G adjacent;
  H cost;
};

// If we decide we need A* with portals, this has an admissable heuristic:
// https://stackoverflow.com/questions/14428331/

} // namespace pathfinding
