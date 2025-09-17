#include "pathfinding.hpp"

#include <algorithm>
#include <queue>

#include "defines.hpp"

const int pathfinding::Dijkstra::infinity = 1 << 30;

void pathfinding::Dijkstra::set(Index xy, int value) {
  if (xy[0] < 0 || xy[1] >= width || xy[1] < 0 || xy[1] >= height) {
    return;
  }

  distance[xy] = value;
}

void pathfinding::Dijkstra::scan(void) {
  auto lambda = [&](const auto &lhs, const auto &rhs) {
    return distance[lhs] > distance[rhs];
  };
  std::priority_queue<Index, std::vector<Index>, decltype(lambda)> queue(
      lambda);
  for (auto x = 0; x < width; x++) {
    for (auto y = 0; y < height; y++) {
      queue.push({x, y});
    }
  }

  while (!queue.empty()) {
    auto next = queue.top();
    queue.pop();
    auto value = distance[next];
    if (value == infinity) {
      return;
    }

    for (auto &dir : directions) {
      auto x = next[0] + dir[0];
      auto y = next[1] + dir[1];
      if (x < 0 || x >= width || y < 0 || y >= height) {
        continue;
      }
      if (value + cost[{x, y}] < distance[{x, y}]) {
        distance[{x, y}] = value + cost[{x, y}];
        queue.push({x, y});
        cameFrom[{x, y}] = next;
      }
    }
  }
}

pathfinding::Index pathfinding::Dijkstra::roll(int x, int y) const {
  auto ret = Index{x, y};
  auto value = distance[{x, y}];

  for (auto &dir : directions) {
    auto x_ = x + dir[0];
    auto y_ = y + dir[1];
    if (x_ < 0 || x_ >= width || y_ < 0 || y_ >= height) {
      continue;
    }
    if (distance[{x_, y_}] < value) {
      value = distance[{x_, y_}];
      ret = {x_, y_};
    }
  }

  return ret;
}

pathfinding::Dijkstra &pathfinding::Dijkstra::operator*=(float scale) {
  for (auto &m : distance) {
    if (m != infinity) {
      m = (int)((float)m * scale);
    }
  }

  return *this;
}

pathfinding::Index pathfinding::Dijkstra::findMin(void) const {
  auto goal = Index{0, 0};
  auto best = distance[goal];
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < height; x++) {
      auto value = distance[{x, y}];
      if (value < best) {
        best = value;
        goal = {x, y};
      }
    }
  }

  return goal;
}

pathfinding::Index pathfinding::Dijkstra::findMax(void) const {
  auto best = -infinity;
  auto goal = Index{0, 0};
  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < height; x++) {
      auto value = distance[{x, y}];
      if (value >= infinity)
        continue;
      if (value > best) {
        best = value;
        goal = {x, y};
      }
    }
  }

  assert(best > 0);

  return goal;
}

std::vector<pathfinding::Index>
pathfinding::Dijkstra::pathDown(Index start) const {
  auto path = std::vector<Index>();
  auto current = start;
  while (true) {
    auto minValue = distance[current];
    auto best = current;
    for (auto &dir : directions) {
      auto newLoc = std::array{current[0] + dir[0], current[1] + dir[1]};
      if (distance[newLoc] < minValue) {
        best = newLoc;
        minValue = distance[newLoc];
      }
    }

    if (best == current) {
      break;
    }
    path.push_back(best);
    current = best;
  }

  std::reverse(path.begin(), path.end());
  return path;
}

std::vector<pathfinding::Index>
pathfinding::Dijkstra::pathUp(Index start, Index goal) const {
  assert(distance[goal] > distance[start]);
  auto path = std::vector<Index>{};
  auto current = goal;
  while (current != start) {
    path.push_back(current);
    current = cameFrom[current];
  }
  return path;
}
