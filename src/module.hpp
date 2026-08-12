#pragma once

#include <flecs.h>

struct Module {
  Module(flecs::world ecs);
};

struct QueryModule {
  QueryModule(flecs::world ecs) { ecs.module<QueryModule>("Queries"); }
};
