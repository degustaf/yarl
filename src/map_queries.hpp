#pragma once

#include <cassert>
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
  Scent,
  Fighter,
  Named,
  Flammable,
  NamedFighter,
};

template <auto q> static inline constexpr auto queryName() {
  if constexpr (q == positionQuery::Position) {
    return "position";
  } else if constexpr (q == positionQuery::Openable) {
    return "positionOpenable";
  } else if constexpr (q == positionQuery::Portal) {
    return "positionPortal";
  } else if constexpr (q == positionQuery::Item) {
    return "positionItem";
  } else if constexpr (q == positionQuery::NewItem) {
    return "positionNewItem";
  } else if constexpr (q == positionQuery::Blocks) {
    return "positionBlocks";
  } else if constexpr (q == positionQuery::Ai) {
    return "positionAi";
  } else if constexpr (q == positionQuery::Describable) {
    return "positionDescribable";
  } else if constexpr (q == positionQuery::Scent) {
    return "positionScent";
  } else if constexpr (q == positionQuery::Fighter) {
    return "positionFighter";
  } else if constexpr (q == positionQuery::Named) {
    return "positionNamed";
  } else if constexpr (q == positionQuery::Flammable) {
    return "positionFlammable";
  } else if constexpr (q == positionQuery::NamedFighter) {
    return "positionNamedFighter";
  } else {
    static_assert(false);
  }
}

template <auto qName>
static inline flecs::query<const Position> mapQuery(flecs::world ecs,
                                                    flecs::entity map) {
  constexpr const char *qname = queryName<qName>();
  static const auto n = std::string("Queries::") + qname;
  static const auto name = n.c_str();
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
      return ecs.query_builder<const Position>(name)
          .with<Describable>()
          .with(flecs::ChildOf, map)
          .build();
    } else {
      assert(false);
    }
  }
}

template <typename T> static inline constexpr const char *typeName();
template <> inline constexpr const char *typeName<Named>() { return "Named"; }
template <> inline constexpr const char *typeName<const Named>() {
  return "ConstNamed";
}
template <> inline constexpr const char *typeName<Scent>() { return "Scent"; }
template <> inline constexpr const char *typeName<Fighter>() {
  return "Fighter";
}

template <auto qName, typename T>
static inline flecs::query<const Position, const T>
mapQuery(flecs::world ecs, flecs::entity map) {
  constexpr const char *qname = queryName<qName>();
  static const auto n = std::string("Queries::") + typeName<T>() + qname;
  static const auto name = n.c_str();
  auto e = ecs.lookup(name);
  if (e) {
    auto q = ecs.query(e);
    return flecs::query<const Position, const T>(q);
  } else {
    if constexpr ((qName == positionQuery::Scent) ||
                  (qName == positionQuery::Fighter) ||
                  (qName == positionQuery::Named)) {
      return ecs.query_builder<const Position, const T>(name)
          .with(flecs::ChildOf, map)
          .build();
    } else if constexpr (qName == positionQuery::Flammable) {
      return ecs.query_builder<const Position, const Named>(name)
          .template with<Flammable>()
          .with(flecs::ChildOf, map)
          .build();
    } else {
      static_assert(false);
    }
  }
}

template <auto qName, typename T, typename U>
static inline flecs::query<const Position, T, U> mapQuery(flecs::world ecs,
                                                          flecs::entity map) {
  constexpr const char *qname = queryName<qName>();
  static const auto n =
      std::string("Queries::") + typeName<T>() + typeName<U>() + qname;
  static const auto name = n.c_str();
  auto e = ecs.lookup(name);
  if (e) {
    auto q = ecs.query(e);
    return flecs::query<const Position, T, U>(q);
  } else {
    if constexpr (qName == positionQuery::NamedFighter) {
      return ecs.query_builder<const Position, Fighter, const Named>(name)
          .with(flecs::ChildOf, map)
          .build();
    } else {
      static_assert(false);
    }
  }
}
