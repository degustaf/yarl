#pragma once

#include <cassert>

#include <flecs.h>

template <typename BaseClass> BaseClass *get(flecs::entity e) {
  // TODO: Blocked by https://github.com/SanderMertens/flecs/issues/1537
  // We should be able to remove this function entirely.
  auto ecs = e.world();
  auto baseEntity = ecs.component<BaseClass>();
  auto q = ecs.query_builder().with(flecs::IsA, baseEntity).build();

  auto Klass = e.null();
  ecs.defer_begin();
  q.each([&](auto comp) {
    if (e.has(comp)) {
      assert(Klass == Klass.null());
      Klass = comp;
    }
  });
  ecs.defer_end();

  if (Klass == e.null()) {
    return nullptr;
  }
  return static_cast<BaseClass *>(e.get_mut(Klass));
};
