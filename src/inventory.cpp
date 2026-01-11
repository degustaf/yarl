#include "inventory.hpp"

#include "actor.hpp"

bool Inventory::hasRoom(flecs::entity e) const {
  auto q =
      e.world().query_builder("module::inventory").with<ContainedBy>(e).build();
  return q.count() < capacity;
}

void drop(flecs::entity item, flecs::entity wearer) {
  item.remove<ContainedBy>(wearer);
  item.set<Position>(wearer.get<Position>());
}

void Taser::apply(flecs::entity target) const {
  if (target.has<Frozen>()) {
    target.get_mut<Frozen>().turns += turns;
  } else {
    target.set<Frozen>({turns});
  }
}
