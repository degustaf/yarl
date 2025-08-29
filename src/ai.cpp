#include "ai.hpp"

#include <algorithm>
#include <cstdlib>
#include <libtcod/console_printing.hpp>
#include <memory>

#include "action.hpp"
#include "actor.hpp"
#include "defines.hpp"
#include "game_map.hpp"
#include "pathfinding.hpp"

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

    auto cost = pathfinding::map<int>(
        std::array<size_t, 2>{(size_t)map.getWidth(), (size_t)map.getHeight()},
        1);
    for (auto y = 0; y < map.getHeight(); y++) {
      for (auto x = 0; x < map.getWidth(); x++) {
        if (!map.isWalkable({x, y})) {
          cost[{(size_t)x, (size_t)y}] = INT_MAX;
        }
      }
    }

    auto q = ecs.query_builder<const Position>("module::blocks")
                 .with<BlocksMovement>()
                 .with(flecs::ChildOf, mapEntity)
                 .build();

    q.each([&](const Position &p) { cost[p] = 10; });

    path = pathfinding::aStar(
        cost, pos, target, [&](const std::array<size_t, 2> &xy) {
          return std::max(std::abs((int)xy[0] - target.x),
                          std::abs((int)xy[1] - target.y));
        });
  }

  if (path.size() > 0) {
    const auto [dest_x, dest_y] = path[path.size() - 1];
    path.pop_back();
    return std::make_unique<MoveAction>(dest_x - pos.x, dest_y - pos.y);
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

    auto msg = tcod::stringf("The %s is no longer confused.",
                             self.get<Named>().name.c_str());
    return std::make_unique<MessageAction>(msg);
  }

  auto rng = TCODRandom::getInstance();
  auto idx = rng->getInt(0, nDirs);
  auto dxy = directions[idx];
  turns_remaining--;
  return std::make_unique<BumpAction>(dxy[0], dxy[1]);
}
