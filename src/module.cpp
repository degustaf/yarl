#include "module.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <libtcod.hpp>

#include "actor.hpp"
#include "ai.hpp"
#include "consumable.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "inventory.hpp"
#include "message_log.hpp"

template <typename Elem, typename Vector = std::vector<Elem>>
flecs::opaque<Vector, Elem> std_vector_support(flecs::world &world) {
  return flecs::opaque<Vector, Elem>()
      .as_type(world.vector<Elem>())

      // Forward elements of std::vector value to serializer
      .serialize([](const flecs::serializer *s, const Vector *data) {
        for (const auto &el : *data) {
          s->value(el);
        }
        return 0;
      })

      // Return vector count
      .count([](const Vector *data) { return data->size(); })

      // Resize contents of vector
      .resize([](Vector *data, size_t size) { data->resize(size); })

      // Ensure element exists, return pointer
      .ensure_element([](Vector *data, size_t elem) {
        if (data->size() <= elem) {
          data->resize(elem + 1);
        }

        return &data->data()[elem];
      });
}

template <typename Elem, std::size_t N, typename Array = std::array<Elem, N>>
flecs::opaque<Array, Elem> std_array_support(flecs::world &world) {
  return flecs::opaque<Array, Elem>()
      .as_type(world.array<Elem>(N))

      // Forward elements of std::array value to serializer
      .serialize([](const flecs::serializer *s, const Array *data) {
        for (const auto &el : *data) {
          s->value(el);
        }
        return 0;
      })

      // Return array count
      .count([](const Array *) { return N; })

      // Ensure element exists, return pointer
      .ensure_element([](Array *data, size_t elem) { return &(*data)[elem]; });
}

module::module(flecs::world ecs) {
  ecs.module<module>("module");

  // STL
  ecs.component<std::string>()
      .opaque(flecs::String) // Opaque type that maps to string
      .serialize([](const flecs::serializer *s, const std::string *data) {
        const char *str = data->c_str();
        return s->value(flecs::String, &str); // Forward to serializer
      })
      .assign_string([](std::string *data, const char *value) {
        *data = value; // Assign new value to std::string
      });

  // TCOD
  ecs.component<tcod::Context>();
  ecs.component<tcod::Console>();
  ecs.component<tcod::ColorRGB>()
      .member<uint8_t>("r")
      .member<uint8_t>("g")
      .member<uint8_t>("b");

  // actor.hpp
  ecs.component<Position>().member<int>("x").member<int>("y");
  ecs.component<RenderOrder>();
  ecs.component<Renderable>()
      .member<int32_t>("ch")
      .member<tcod::ColorRGB>("color")
      .member<RenderOrder>("layer");
  ecs.component<Named>().member<std::string>("name");
  ecs.component<BlocksMovement>();
  ecs.component<Fighter>()
      .member<int>("max_hp")
      .member<int>("_hp")
      .member<int>("defense")
      .member<int>("power");

  // ai.hpp
  ecs.component<Ai>();
  ecs.component<std::array<size_t, 2>>().opaque(std_array_support<size_t, 2>);
  ecs.component<std::vector<std::array<size_t, 2>>>().opaque(
      std_vector_support<std::array<size_t, 2>>);
  ecs.component<HostileAi>()
      .member("path", &HostileAi::path)
      .is_a<Ai>()
      .add(flecs::CanToggle);
  ecs.component<ConfusedAi>()
      .member("turns_remaining", &ConfusedAi::turns_remaining)
      .is_a<Ai>()
      .add(flecs::CanToggle);

  // consumable.hpp
  ecs.component<HealingConsumable>().member<int>("amount");
  ecs.component<LightningDamageConsumable>().member<int>("damage").member<int>(
      "maximumRange");
  ecs.component<ConfusionConsumable>().member<int>("number_of_turns");
  ecs.component<FireballDamageConsumable>().member<int>("damage").member<int>(
      "radius");

  // game_map.hpp
  ecs.component<CurrentMap>();
  ecs.component<Tile>().member<uint8_t>("flags");
  ecs.component<std::vector<Tile>>().opaque(std_vector_support<Tile>);
  ecs.component<GameMap>()
      .member<int>("width")
      .member<int>("height")
      .member<uint32_t>("seed")
      .member<std::vector<Tile>>("tiles");

  // input_handler.hpp
  ecs.component<EventHandler>();

  // inventory.hpp
  ecs.component<Inventory>().member<int>("capacity");
  ecs.component<ContainedBy>().add(flecs::Exclusive);
  ecs.component<Item>();

  // message_log.hpp
  ecs.component<Message>()
      .member<std::string>("plain_text")
      .member<tcod::ColorRGB>("fg")
      .member<int>("count");
  ecs.component<MessageLog>().opaque(std_vector_support<Message>);

  ecs.prefab("orc")
      .set<Renderable>({'o', {63, 127, 63}, RenderOrder::Actor})
      .set<Named>({"Orc"})
      .add<BlocksMovement>()
      .set<HostileAi>({})
      .emplace<Fighter>(10, 0, 3);

  ecs.prefab("troll")
      .set<Renderable>({'T', {0, 127, 0}, RenderOrder::Actor})
      .set<Named>({"Troll"})
      .add<BlocksMovement>()
      .set<HostileAi>({})
      .emplace<Fighter>(16, 1, 4);

  ecs.prefab("healthPotion")
      .set<Renderable>({'!', {127, 0, 255}, RenderOrder::Item})
      .set<Named>({"Health Potion"})
      .add<Item>()
      .set<HealingConsumable>({4});

  ecs.prefab("lightningScroll")
      .set<Renderable>({'~', {255, 255, 0}, RenderOrder::Item})
      .set<Named>({"Lightning Scroll"})
      .add<Item>()
      .set<LightningDamageConsumable>({20, 5});

  ecs.prefab("confusionScroll")
      .set<Renderable>({'~', {207, 63, 255}, RenderOrder::Item})
      .set<Named>({"Confusion Scroll"})
      .add<Item>()
      .set<ConfusionConsumable>({10});

  ecs.prefab("fireballScroll")
      .set<Renderable>({'~', {255, 0, 0}, RenderOrder::Item})
      .set<Named>({"Fireball Scroll"})
      .add<Item>()
      .set<FireballDamageConsumable>({12, 3});
}
