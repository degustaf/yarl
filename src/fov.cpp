#include "actor.hpp"
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
  int dx;
  int dy;
  double startSlope;
  double endSlope;

  Row next(int col = 0) const {
    return {origin, quad, depth + 1, dx + 1, dy + col, startSlope, endSlope};
  };

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

static void scan(GameMap &map, flecs::query<const Position> &q, Row row,
                 int rSquared) {
  auto prev_tile = std::optional<std::array<int, 2>>(std::nullopt);
  for (auto col = (int)std::floor(row.depth * row.startSlope + 0.5);
       col <= (int)std::ceil(row.depth * row.endSlope - 0.5); col++) {
    if (row.dx * row.dx + (row.dy + col) * (row.dy + col) > rSquared)
      continue;
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
      scan(map, q, nextRow, rSquared);
    }
    auto portals = std::vector<Position>{};
    q.each([&](flecs::iter &it, size_t, const Position &p) {
      if (p == row.transform(tile)) {
        auto otherSide = it.pair(1).second();
        portals.push_back(otherSide.get<Position>());
      }
    });
    for (auto p : portals) {
      map.setFov(p, true);
      scan(map, q,
           {p, row.quad, 1, row.dx, row.dy + col, slope(tile),
            slope({tile[0], tile[1] + 1})},
           rSquared);
    }
    prev_tile = tile;
  }
  if (prev_tile && isFloor(map, row, *prev_tile)) {
    scan(map, q, row.next(), rSquared);
  }
}

void computeFov(flecs::entity mapEntity, GameMap &map,
                std::array<int, 2> origin, int maxRadius) {

  for (auto y = 0; y < map.getHeight(); y++) {
    for (auto x = 0; x < map.getWidth(); x++) {
      map.setFov({x, y}, false);
    }
  }

  auto ecs = mapEntity.world();
  auto q = ecs.query_builder<const Position>()
               .with(ecs.component<Portal>(), flecs::Wildcard)
               .with(flecs::ChildOf, mapEntity)
               .build();

  map.setFov(origin, true);

  for (auto quad : quadrants) {
    scan(map, q, {origin, quad, 1, 0, 0, -1.0, 1.0}, maxRadius * maxRadius);
  }
}
