#include "game_map.hpp"
#include <array>
#include <cmath>
#include <optional>

enum class Quadrant {
  North,
  East,
  South,
  West,
};

static constexpr auto quadrants = std::array<Quadrant, 4>{
    Quadrant::North, Quadrant::East, Quadrant::South, Quadrant::West};

struct Row {
  std::array<int, 2> origin;
  Quadrant quad;
  int depth;
  double startSlope;
  double endSlope;

  Row next() const { return {origin, quad, depth + 1, startSlope, endSlope}; };

  std::array<int, 2> transform(std::array<int, 2> tile) const {
    switch (quad) {
    case Quadrant::North:
      return {origin[0] + tile[1], origin[1] - tile[0]};
    case Quadrant::East:
      return {origin[0] + tile[0], origin[1] + tile[1]};
    case Quadrant::South:
      return {origin[0] + tile[1], origin[1] + tile[0]};
    case Quadrant::West:
      return {origin[0] - tile[0], origin[1] + tile[1]};
    }
    assert(false);
    return {-1, -1};
  }

  bool is_symmetric(std::array<int, 2> tile) {
    return tile[1] >= depth * startSlope && tile[1] <= depth * endSlope;
  }
};

static bool isWall(const GameMap &map, const Row &row,
                   std::array<int, 2> tile) {
  return !map.isTransparent(row.transform(tile));
}

static bool isFloor(const GameMap &map, const Row &row,
                    std::array<int, 2> tile) {
  return map.isTransparent(row.transform(tile));
}

static double slope(std::array<int, 2> tile) {
  return (2.0 * tile[1] - 1) / (2.0 * tile[0]);
}

static void scan(GameMap &map, Row row) {
  auto prev_tile = std::optional<std::array<int, 2>>(std::nullopt);
  for (auto col = (int)std::floor(row.depth * row.startSlope + 0.5);
       col <= (int)std::ceil(row.depth * row.endSlope - 0.5); col++) {
    auto tile = std::array{row.depth, col};
    assert(map.inBounds(row.transform(tile)));
    if (isWall(map, row, tile) || row.is_symmetric(tile)) {
      map.setFov(row.transform(tile), true);
    }
    if (prev_tile && isWall(map, row, *prev_tile) && isFloor(map, row, tile)) {
      row.startSlope = slope(tile);
    }
    if (prev_tile && isFloor(map, row, *prev_tile) && isWall(map, row, tile)) {
      auto nextRow = row.next();
      nextRow.endSlope = slope(tile);
      scan(map, nextRow);
    }
    prev_tile = tile;
  }
  if (prev_tile && isFloor(map, row, *prev_tile)) {
    scan(map, row.next());
  }
}

void computeFov(GameMap &map, std::array<int, 2> origin, int maxRadius) {
  map.setFov(origin, true);

  for (auto quad : quadrants) {
    scan(map, {origin, quad, 1, -1.0, 1.0});
  }

  for (auto y = 0; y < map.getHeight(); y++) {
    auto dy = origin[1] - y;
    for (auto x = 0; x < map.getWidth(); x++) {
      auto dx = origin[0] - x;
      if (dx * dx + dy * dy > maxRadius * maxRadius) {
        map.setFov({x, y}, false);
      }
    }
  }
}
