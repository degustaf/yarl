#pragma once

#include <array>
#include <cassert>
#include <vector>

#include <flecs.h>

#include "game_map.hpp"
#include "random.hpp"

static constexpr auto MAX_ROOMS = 30;

struct RectangularRoom {
  RectangularRoom(int x, int y, int width, int height)
      : x1(x), y1(y), x2(x + width), y2(y + height) {};
  RectangularRoom() : x1(0), y1(0), x2(0), y2(0) {};

  std::array<int, 2> center(void) const {
    return {(x1 + x2) / 2, (y1 + y2) / 2};
  };

  void carveOut(GameMap &map) const {
    for (auto y = y1 + 1; y < y2; y++) {
      for (auto x = x1 + 1; x < x2; x++) {
        map.carveOut(x, y);
      }
    }
  }

  inline bool intersect(const RectangularRoom &other) const {
    return x1 <= other.x2 && x2 >= other.x1 && y1 <= other.y2 && y2 >= other.y1;
  };

  int x1;
  int y1;
  int x2;
  int y2;
};

struct MaxByFloor {
  int minFloor;
  int count;
};

static constexpr auto max_items_by_floor =
    std::array<MaxByFloor, 2>{MaxByFloor{1, 1}, {4, 2}};

static constexpr auto max_monsters_by_floor =
    std::array<MaxByFloor, 1>{MaxByFloor{1, 2}};

template <size_t N>
static int getMaxValueForFloor(const std::array<MaxByFloor, N> &maxByFloor,
                               int floor) {
  auto currentCount = 0;
  for (const auto &f : maxByFloor) {
    if (f.minFloor > floor) {
      break;
    } else {
      assert(f.count >= currentCount);
      currentCount = f.count;
    }
  }
  return currentCount;
}

struct WeightData {
  int minFloor;
  int weight;
};

struct FloorWeights {
  std::vector<WeightData> data;
};

struct WeightsByFloor {
  int minFloor;
  int weight;
  std::string name;
};

template <typename T>
static const std::string get_entity_at_random(const T &weights, int floor,
                                              Random &rng) {
  auto totalWeight = 0;
  for (const auto &w : weights) {
    if (w.minFloor <= floor) {
      totalWeight += w.weight;
    }
  }

  auto choice = rng.getInt(1, totalWeight);
  for (const auto &w : weights) {
    if (w.minFloor <= floor) {
      if (choice <= w.weight) {
        return w.name;
      }
      choice -= w.weight;
    }
  }
  assert(false);
  return "";
}

std::vector<WeightsByFloor> buildMonsterWeights(flecs::world);
std::vector<WeightsByFloor> buildItemWeights(flecs::world);
