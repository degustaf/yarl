#include "map_shared.hpp"

std::vector<WeightsByFloor> buildMonsterWeights(flecs::world ecs) {
  auto ret = std::vector<WeightsByFloor>{};
  auto q = ecs.query_builder<const FloorWeights>()
               .with<Fighter>()
               .with(flecs::Prefab)
               .build();

  q.each([&ret](flecs::entity e, const FloorWeights &f) {
    const auto qualifiedName =
        std::string(e.parent().name()) + "::" + std::string(e.name());
    for (auto &wd : f.data) {
      ret.push_back({wd.minFloor, wd.weight, qualifiedName});
    }
  });

  return ret;
}

std::vector<WeightsByFloor> buildItemWeights(flecs::world ecs) {
  auto ret = std::vector<WeightsByFloor>{};
  auto q = ecs.query_builder<const FloorWeights>()
               .without<Fighter>()
               .with(flecs::Prefab)
               .build();

  q.each([&ret](flecs::entity e, const FloorWeights &f) {
    const auto qualifiedName =
        std::string(e.parent().name()) + "::" + std::string(e.name());
    for (auto &wd : f.data) {
      ret.push_back({wd.minFloor, wd.weight, qualifiedName});
    }
  });

  return ret;
}
