#include "inventory.hpp"

bool Inventory::hasRoom(flecs::entity e) const {
  auto q =
      e.world().query_builder("module::inventory").with<ContainedBy>(e).build();
  return q.count() < capacity;
}

void drop(flecs::entity item, flecs::entity wearer) {
  item.remove<ContainedBy>(wearer);
  item.set<Position>(wearer.get<Position>());
}
