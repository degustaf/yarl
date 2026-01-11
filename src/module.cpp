#include "module.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "actor.hpp"
#include "ai.hpp"
#include "color.hpp"
#include "console.hpp"
#include "consumable.hpp"
#include "engine.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "message_log.hpp"
#include "renderer.hpp"
#include "scent.hpp"

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

template <typename Elem>
flecs::opaque<std::optional<Elem>, Elem>
std_optional_support(flecs::world &world) {
  return flecs::opaque<std::optional<Elem>, Elem>()
      .as_type(world.vector<Elem>())
      .serialize(
          [](const flecs::serializer *s, const std::optional<Elem> *data) {
            if (*data) {
              s->value(**data);
            }
            return 0;
          })
      .count([](const std::optional<Elem> *data) -> size_t {
        return *data ? 1 : 0;
      })
      .resize([](std::optional<Elem> *data, size_t size) {
        switch (size) {
        case 0:
          *data = std::nullopt;
          break;
        case 1:
          *data = Elem();
          break;
        default:
          assert(false);
        }
      })
      .ensure_element(
          [](std::optional<Elem> *data, size_t) { return &**data; });
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

  // actor.hpp
  ecs.component<Position>().member<int>("x").member<int>("y");
  ecs.component<RenderOrder>();
  ecs.component<std::optional<color::RGB>>().opaque(
      std_optional_support<color::RGB>);
  ecs.component<Renderable>()
      .member<int32_t>("ch")
      .member<color::RGB>("fg")
      .member<std::optional<color::RGB>>("bg")
      .member<RenderOrder>("layer");
  ecs.component<Named>().member<std::string>("name");
  ecs.component<BlocksMovement>();
  ecs.component<BlocksFov>();
  ecs.component<Openable>();
  ecs.component<Fountain>();
  ecs.component<Fighter>()
      .member<int>("max_hp")
      .member<int>("_hp")
      .member<int>("defense")
      .member<int>("power");
  ecs.component<Regenerator>().member<int>("healTurns").member<int>("turns");
  ecs.component<OnDeath>();
  ecs.component<Frozen>().member<int>("turns");

  // ai.hpp
  ecs.component<Ai>();
  ecs.component<std::array<int, 2>>().opaque(std_array_support<int, 2>);
  ecs.component<std::vector<std::array<int, 2>>>().opaque(
      std_vector_support<std::array<int, 2>>);
  ecs.component<HostileAi>()
      .member("path", &HostileAi::path)
      .is_a<Ai>()
      .add(flecs::CanToggle);
  ecs.component<ConfusedAi>()
      .member("turns_remaining", &ConfusedAi::turns_remaining)
      .is_a<Ai>()
      .add(flecs::CanToggle);

  // color.hpp
  ecs.component<color::RGB>()
      .member<uint8_t>("r")
      .member<uint8_t>("g")
      .member<uint8_t>("b");
  ecs.component<color::RGBA>()
      .member<uint8_t>("r")
      .member<uint8_t>("g")
      .member<uint8_t>("b")
      .member<uint8_t>("a");

  // console.hpp
  ecs.component<Console>();

  // consumable.hpp
  ecs.component<Consumable>();
  ecs.component<HealingConsumable>().member<int>("amount").is_a<Consumable>();
  ecs.component<DeodorantConsumable>()
      .member<float>("amount")
      .is_a<Consumable>();
  ecs.component<LightningDamageConsumable>()
      .member<int>("damage")
      .member<int>("maximumRange")
      .is_a<Consumable>();
  ecs.component<ConfusionConsumable>()
      .member<int>("number_of_turns")
      .is_a<Consumable>();
  ecs.component<FireballDamageConsumable>()
      .member<int>("damage")
      .member<int>("radius")
      .is_a<Consumable>();
  ecs.component<MagicMappingConsumable>().is_a<Consumable>();
  ecs.component<RopeConsumable>().is_a<Consumable>();
  ecs.component<TransporterConsumable>().is_a<Consumable>();

  // engine.hpp
  ecs.component<Seed>().member<uint32_t>("seed");

  // renderer.hpp
  ecs.component<SDLData>();

  // scent.hpp
  ecs.component<ScentType>();
  ecs.component<Scent>().member<ScentType>("type").member<float>("power");
  ecs.component<ScentWarning>().member<bool>("warned");
  ecs.component<ScentOnDeath>()
      .member("type", &ScentOnDeath::type)
      .member("power", &ScentOnDeath::power)
      .is_a<OnDeath>();
  ecs.component<Smeller>().member<float>("threshold");
  ecs.component<ScentConsumable>().member<Scent>("scent").is_a<Consumable>();

  // game_map.hpp
  ecs.component<CurrentMap>().add(flecs::Exclusive);
  ecs.component<Tile>().member<uint8_t>("flags");
  ecs.component<std::vector<Tile>>().opaque(std_vector_support<Tile>);
  ecs.component<std::vector<Scent>>().opaque(std_vector_support<Scent>);
  ecs.component<GameMap>()
      .member<int>("width")
      .member<int>("height")
      .member<int>("level")
      .member<std::vector<Tile>>("tiles")
      .member<std::vector<Scent>>("scent");

  // input_handler.hpp
  ecs.component<EventHandler>();

  // inventory.hpp
  ecs.component<Inventory>().member<int>("capacity");
  ecs.component<ContainedBy>().add(flecs::Exclusive);
  ecs.component<Item>();
  ecs.component<EquipmentType>();
  ecs.component<Equippable>()
      .member<EquipmentType>("type")
      .member<int>("power_bonus")
      .member<int>("defense_bonus");
  ecs.component<Armor>().add(flecs::Exclusive);
  ecs.component<Weapon>().add(flecs::Exclusive);
  ecs.component<Ranged>().member<int>("range");
  ecs.component<Taser>().member<int>("turns");

  // level.hpp
  ecs.component<XP>().member<int>("given");
  ecs.component<Level>().member<int>("current").member<int>("xp");

  // message_log.hpp
  ecs.component<Message>()
      .member<std::string>("plain_text")
      .member<color::RGB>("fg")
      .member<int>("count");
  ecs.component<MessageLog>().opaque(std_vector_support<Message>);

  ecs.prefab("orc")
      .set<Renderable>(
          {'o', color::desaturatedGreen, std::nullopt, RenderOrder::Actor})
      .set<Named>({"Orc"})
      .add<BlocksMovement>()
      .set<HostileAi>({})
      .emplace<Fighter>(10, 0, 3)
      .set<XP>({35});

  ecs.prefab("troll")
      .set<Renderable>(
          {'T', color::darkerGreen, std::nullopt, RenderOrder::Actor})
      .set<Named>({"Troll"})
      .add<BlocksMovement>()
      .set<HostileAi>({})
      .emplace<Fighter>(16, 1, 4)
      .set<XP>({100});

  ecs.prefab("cysts")
      .set<Renderable>({0xB6, color::black, std::nullopt, RenderOrder::Actor})
      .set<Named>({"Fiend cysts"})
      .add<BlocksMovement>()
      .emplace<ScentOnDeath>(ScentType::fiend, 100.0f)
      .emplace<Fighter>(1, 0, 0);

  ecs.prefab("corpse")
      .set<Renderable>(
          {'%', color::darkerRed, std::nullopt, RenderOrder::Actor})
      .set<Named>({"Rotting corpse"})
      .set<Scent>({ScentType::decay, 1000});

  ecs.prefab("healthPotion")
      .set<Renderable>({'!', {127, 0, 255}, std::nullopt, RenderOrder::Item})
      .set<Named>({"Vitamyn syringe"})
      .add<Item>()
      .set<HealingConsumable>({4});

  ecs.prefab("deodorant")
      .set<Renderable>({'!', {255, 50, 50}, std::nullopt, RenderOrder::Item})
      .set<Named>({"Deodorant"})
      .add<Item>()
      .set<DeodorantConsumable>({25});

  ecs.prefab("lightningScroll")
      .set<Renderable>({'~', color::yellow, std::nullopt, RenderOrder::Item})
      .set<Named>({"Lightning Scroll"})
      .add<Item>()
      .set<LightningDamageConsumable>({20, 5});

  ecs.prefab("confusionScroll")
      .set<Renderable>({'~', {207, 63, 255}, std::nullopt, RenderOrder::Item})
      .set<Named>({"Confusion Scroll"})
      .add<Item>()
      .set<ConfusionConsumable>({10});

  ecs.prefab("fireballScroll")
      .set<Renderable>({'~', color::red, std::nullopt, RenderOrder::Item})
      .set<Named>({"Fireball Scroll"})
      .add<Item>()
      .set<FireballDamageConsumable>({12, 3});

  ecs.prefab("dung")
      .set<Renderable>({'~', color::brown, std::nullopt, RenderOrder::Item})
      .set<Named>({"Fiend dung"})
      .add<Item>()
      .set<ScentConsumable>({{ScentType::fiend, 100}});

  ecs.prefab("mapper")
      .set<Renderable>(
          {0x00A1, color::lightGrey, std::nullopt, RenderOrder::Item})
      .set<Named>({"Mapper"})
      .add<Item>()
      .add<MagicMappingConsumable>();

  ecs.prefab("transporter")
      .set<Renderable>(
          {0x00A1, color::lightGrey, std::nullopt, RenderOrder::Item})
      .set<Named>({"Transporter"})
      .add<Item>()
      .add<TransporterConsumable>();

  ecs.prefab("rope")
      .set<Renderable>(
          {0x2320, color::lightGrey, std::nullopt, RenderOrder::Item})
      .set<Named>({"Rope"})
      .add<Item>()
      .add<RopeConsumable>();

  ecs.prefab("dagger")
      .set<Renderable>({'/', {0, 191, 255}, std::nullopt, RenderOrder::Item})
      .set<Named>({"Dagger"})
      .add<Item>()
      .set<Equippable>({EquipmentType::Weapon, 2, 0});

  ecs.prefab("sword")
      .set<Renderable>({'/', {0, 191, 255}, std::nullopt, RenderOrder::Item})
      .set<Named>({"Sword"})
      .add<Item>()
      .set<Equippable>({EquipmentType::Weapon, 4, 0});

  ecs.prefab("leatherArmor")
      .set<Renderable>({'[', {139, 69, 19}, std::nullopt, RenderOrder::Item})
      .set<Named>({"Leather armor"})
      .add<Item>()
      .set<Equippable>({EquipmentType::Armor, 0, 1});

  ecs.prefab("chainMail")
      .set<Renderable>({'[', {139, 69, 19}, std::nullopt, RenderOrder::Item})
      .set<Named>({"Chain mail"})
      .add<Item>()
      .set<Equippable>({EquipmentType::Armor, 0, 3});

  ecs.prefab("22")
      .set<Renderable>({0x2310, {0, 191, 255}, std::nullopt, RenderOrder::Item})
      .set<Named>({".22"})
      .add<Item>()
      .set<Ranged>({8})
      .set<Equippable>({EquipmentType::Weapon, 4, 0});

  ecs.prefab("45")
      .set<Renderable>({0x2310, {0, 191, 255}, std::nullopt, RenderOrder::Item})
      .set<Named>({".45"})
      .add<Item>()
      .set<Ranged>({8})
      .set<Equippable>({EquipmentType::Weapon, 8, 0});

  ecs.prefab("taser")
      .set<Renderable>({0x2310, {0, 191, 255}, std::nullopt, RenderOrder::Item})
      .set<Named>({"taser"})
      .add<Item>()
      .set<Taser>({3})
      .set<Equippable>({EquipmentType::Weapon, 0, 0});

  ecs.prefab("yendor")
      .set<Renderable>({0x263C, color::orange, std::nullopt, RenderOrder::Item})
      .set<Named>({"Laser of Yendor"})
      .add<Item>()
      .set<Ranged>({8})
      .set<Equippable>({EquipmentType::Weapon, 50, 0});

  ecs.prefab("door")
      .set<Renderable>(
          {'+', color::black, color::darkerRed, RenderOrder::Actor, false})
      .set<Named>({"door"})
      .add<Openable>();

  ecs.prefab("fountain")
      .set<Renderable>(
          {'&', color::blue, std::nullopt, RenderOrder::Actor, false})
      .set<Named>({"fountain"})
      .add<BlocksMovement>()
      .add<Fountain>();
}
