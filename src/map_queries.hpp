#pragma once

#include <flecs.h>

#include "ai.hpp"
#include "game_map.hpp"
#include "inventory.hpp"
#include "position.hpp"

enum class positionQuery {
  Position,
  Openable,
  Portal,
  Item,
  NewItem,
  Blocks,
  Ai,
  Describable,
};

template <auto q> static inline constexpr auto queryName() {
  if constexpr (q == positionQuery::Position) {
    return "module::position";
  } else if constexpr (q == positionQuery::Openable) {
    return "module::positionOpenable";
  } else if constexpr (q == positionQuery::Portal) {
    return "module::positionPortal";
  } else if constexpr (q == positionQuery::Item) {
    return "module::positionItem";
  } else if constexpr (q == positionQuery::NewItem) {
    return "module::positionNewItem";
  } else if constexpr (q == positionQuery::Blocks) {
    return "module::positionBlocks";
  } else if constexpr (q == positionQuery::Ai) {
    return "module::positionAi";
  } else if constexpr (q == positionQuery::Describable) {
    return "module::positionDescribable";
  } else {
    return "";
  }
}

template <auto qName>
static inline flecs::query<const Position> mapQuery(flecs::world ecs,
                                                    flecs::entity map) {
  constexpr const char *name = queryName<qName>();
  auto e = ecs.lookup(name);
  if (e) {
    auto q = ecs.query(e);
    return flecs::query<const Position>(q);
  } else {
    if constexpr (qName == positionQuery::Position) {
      return ecs.query_builder<const Position>(name)
          .with(flecs::ChildOf, map)
          .build();
    } else if constexpr (qName == positionQuery::Openable) {
      return ecs.query_builder<const Position>(name)
          .with<Openable>()
          .with(flecs::ChildOf, map)
          .cached()
          .build();
    } else if constexpr (qName == positionQuery::Portal) {
      return ecs.query_builder<const Position>(name)
          .with(ecs.component<Portal>(), flecs::Wildcard)
          .with(flecs::ChildOf, map)
          .cached()
          .build();
    } else if constexpr (qName == positionQuery::Item) {
      return ecs.query_builder<const Position>(name)
          .with<Item>()
          .with(flecs::ChildOf, map)
          .cached()
          .build();
    } else if constexpr (qName == positionQuery::NewItem) {
      return ecs.query_builder<const Position>(name)
          .with<Item>()
          .without<Dropped>()
          .with(flecs::ChildOf, map)
          .cached()
          .build();
    } else if constexpr (qName == positionQuery::Blocks) {
      return ecs.query_builder<const Position>(name)
          .with(flecs::ChildOf, map)
          .with<BlocksMovement>()
          .build();
    } else if constexpr (qName == positionQuery::Ai) {
      return ecs.query_builder<const Position>(name)
          .with(flecs::ChildOf, map)
          .with<Ai>()
          .build();
    } else if constexpr (qName == positionQuery::Describable) {
      return ecs.query_builder<const Position>()
          .with<Describable>()
          .with(flecs::ChildOf, map)
          .build();
    } else {
      return ecs.query_builder<const Position>(name).build();
    }
  }
}
