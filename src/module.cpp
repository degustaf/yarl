#include "module.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "actor.hpp"
#include "ai.hpp"
#include "audio.hpp"
#include "blood.hpp"
#include "books.hpp"
#include "color.hpp"
#include "console.hpp"
#include "consumable.hpp"
#include "engine.hpp"
#include "game_map.hpp"
#include "input_handler.hpp"
#include "inventory.hpp"
#include "level.hpp"
#include "map_shared.hpp"
#include "message_log.hpp"
#include "position.hpp"
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
          if (!data->has_value()) {
            *data = Elem();
          }
          break;
        default:
          assert(false);
        }
      })
      .ensure_element([](std::optional<Elem> *data, size_t size) {
        (void)size;
        if (!data->has_value()) {
          *data = Elem();
        }
        return &data->value();
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

Module::Module(flecs::world ecs) {
  ecs.module<Module>("PFs");

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
  ecs.component<std::vector<int>>().opaque(std_vector_support<int>);
  ecs.component<std::array<int, 2>>().opaque(std_array_support<int, 2>);
  ecs.component<std::vector<std::array<int, 2>>>().opaque(
      std_vector_support<std::array<int, 2>>);
  ecs.component<std::vector<std::string>>().opaque(
      std_vector_support<std::string>);
  //
  // audio.hpp
  ecs.component<SDLAudio>();

  // color.hpp
  ecs.component<color::RGBA>()
      .member<uint8_t>("r")
      .member<uint8_t>("g")
      .member<uint8_t>("b")
      .member<uint8_t>("a");

  // actor.hpp
  ecs.component<Flying>();
  ecs.component<Invisible>().member<bool>("paused");
  ecs.component<RenderOrder>();
  ecs.component<std::optional<color::RGBA>>().opaque(
      std_optional_support<color::RGBA>);
  ecs.component<Renderable>()
      .member<int32_t>("ch")
      .member<color::RGBA>("fg")
      .member<std::optional<color::RGBA>>("bg")
      .member<RenderOrder>("layer");
  ecs.component<Named>().member<std::string>("name");
  ecs.component<Fighter>()
      .member<int>("max_hp")
      .member<int>("_hp")
      .member<int>("defense")
      .member<int>("power");
  ecs.component<Splitter>().member<int>("minHP");
  ecs.component<Regenerator>().member<int>("healTurns").member<int>("turns");
  ecs.component<OnDeath>();
  ecs.component<Frozen>();
  ecs.component<Temporary>().member<int>("turns").member<flecs::entity>(
      "component");
  ecs.component<Describable>();

  // ai.hpp
  ecs.component<Ai>();
  ecs.component<HostileAi>()
      .member("path", &HostileAi::path)
      .is_a<Ai>()
      .add(flecs::CanToggle);
  ecs.component<ConfusedAi>()
      .member("turns_remaining", &ConfusedAi::turns_remaining)
      .is_a<Ai>()
      .add(flecs::CanToggle);
  ecs.component<WanderAi>()
      .member("memory", &WanderAi::memory)
      .is_a<Ai>()
      .add(flecs::CanToggle);

  // blood.hpp
  ecs.component<BloodDrop>();

  // books.hpp
  ecs.component<Book>()
      .member<std::string>("title")
      .member<std::vector<std::string>>("body");

  // console.hpp
  ecs.component<Console>();

  // consumable.hpp
  ecs.component<Consumable>();
  ecs.component<HealingConsumable>()
      .member("amount", &HealingConsumable::amount)
      .is_a<Consumable>();
  ecs.component<DeodorantConsumable>()
      .member("amount", &DeodorantConsumable::amount)
      .is_a<Consumable>();
  ecs.component<LightningDamageConsumable>()
      .member("damage", &LightningDamageConsumable::damage)
      .member("maximumRange", &LightningDamageConsumable::maximumRange)
      .member("animation_ch", &LightningDamageConsumable::animation_ch)
      .member("animation_color", &LightningDamageConsumable::animation_color)
      .is_a<Consumable>();
  ecs.component<ConfusionConsumable>()
      .member("number_of_turns", &ConfusionConsumable::number_of_turns)
      .is_a<Consumable>();
  ecs.component<FireballDamageConsumable>()
      .member("damage", &FireballDamageConsumable::damage)
      .member("radius", &FireballDamageConsumable::radius)
      .member("animation_ch", &FireballDamageConsumable::animation_ch)
      .member("edges", &FireballDamageConsumable::edges)
      .member("center", &FireballDamageConsumable::center)
      .is_a<Consumable>();
  ecs.component<MagicMappingConsumable>().is_a<Consumable>();
  ecs.component<RopeConsumable>().is_a<Consumable>();
  ecs.component<TransporterConsumable>().is_a<Consumable>();
  ecs.component<LightConsumable>()
      .member("turns", &LightConsumable::turns)
      .member("innerRadius", &LightConsumable::innerRadius)
      .member("outerRadius", &LightConsumable::outerRadius)
      .member("decayFactor", &LightConsumable::decayFactor)
      .is_a<Consumable>();

  // engine.hpp
  ecs.component<Seed>().member<uint32_t>("seed");
  ecs.component<Turn>().member<int64_t>("turn");

  // position.hpp
  ecs.component<Position>().member<int>("x").member<int>("y");
  ecs.component<FPosition>();
  ecs.component<Velocity>();
  ecs.component<RadialLimit>();
  ecs.component<AttackAnimation>();
  ecs.component<MoveAnimation>();
  ecs.component<Fade>();
  ecs.component<DisappearOnHit>();
  ecs.component<BlocksMovement>();
  ecs.component<Trauma>();

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
  ecs.component<ScentConsumable>()
      .member("scent", &ScentConsumable::scent)
      .is_a<Consumable>();

  // game_map.hpp
  ecs.component<BlocksMovement>();
  ecs.component<BlocksFov>();
  ecs.component<Openable>();
  ecs.component<Fountain>();
  ecs.component<Portal>().add(flecs::Symmetric);
  ecs.component<Light>()
      .member<int>("innerRadius")
      .member<int>("outerRadius")
      .member<float>("decayFactor");
  ecs.component<CurrentMap>().add(flecs::Exclusive);
  ecs.component<Tile>()
      .member<uint16_t>("flags")
      .member<Scent>("scent")
      .member<float>("luminosity");
  ecs.component<std::vector<Tile>>().opaque(std_vector_support<Tile>);
  ecs.component<std::vector<Scent>>().opaque(std_vector_support<Scent>);
  ecs.component<GameMap>()
      .member<int>("width")
      .member<int>("height")
      .member<int>("level")
      .member<bool>("lit")
      .member<std::vector<Tile>>("tiles");

  // input_handler.hpp
  ecs.component<InputHandler>();
  ecs.component<std::unique_ptr<InputHandler>>();

  // inventory.hpp
  ecs.component<Inventory>().member<int>("capacity");
  ecs.component<ContainedBy>().add(flecs::Exclusive);
  ecs.component<Item>();
  ecs.component<Flammable>();
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

  // map_shared.hpp
  ecs.component<WeightData>().member<int>("minFloor").member<int>("weight");
  ecs.component<std::vector<WeightData>>().opaque(
      std_vector_support<WeightData>);
  ecs.component<FloorWeights>().member<std::vector<WeightData>>("data");
  ecs.component<WeightsByFloor>()
      .member<int>("minFloor")
      .member<int>("weight")
      .member<std::string>("name");

  // message_log.hpp
  ecs.component<Message>()
      .member<std::string>("plain_text")
      .member<color::RGBA>("fg")
      .member<int>("count");
  ecs.component<MessageLog>().opaque(std_vector_support<Message>);

  ecs.prefab("cat")
      .set<Renderable>(
          {'f', Colors::background, std::nullopt, RenderOrder::Actor})
      .set<Named>({"cat"})
      .add<Describable>()
      .add<BlocksMovement>();

  ecs.prefab("door")
      .set<Renderable>(
          {'+', Colors::background, Colors::door, RenderOrder::Actor, false})
      .set<Named>({"door"})
      .add<Openable>();
}

Colors::Colors(flecs::world ecs) {
  ecs.module<Colors>("Colors");

  ecs.component<color::RGBA>()
      .member<uint8_t>("r")
      .member<uint8_t>("g")
      .member<uint8_t>("b")
      .member<uint8_t>("a");

  auto script = std::filesystem::path("assets/colors.flecs");
  ecs.script().filename(script.string().c_str()).run();

  ecs.get_const_var("text", text);
  ecs.get_const_var("background", background);

  ecs.get_const_var("impossible", impossible);
  ecs.get_const_var("playerAtk", playerAtk);
  ecs.get_const_var("monsterAtk", monsterAtk);
  ecs.get_const_var("playerDie", playerDie);
  ecs.get_const_var("monsterDie", monsterDie);
  ecs.get_const_var("descend", descend);
  ecs.get_const_var("healthRecovered", healthRecovered);
  ecs.get_const_var("needsTarget", needsTarget);
  ecs.get_const_var("statusEffectApplied", statusEffectApplied);
  ecs.get_const_var("welcomeText", welcomeText);
  ecs.get_const_var("jump", jump);

  ecs.get_const_var("menu_border", menu_border);
  ecs.get_const_var("menu_background", menu_background);
  ecs.get_const_var("menu_title", menu_title);
  ecs.get_const_var("menu_text", menu_text);

  ecs.get_const_var("barText", barText);
  ecs.get_const_var("barFilled", barFilled);
  ecs.get_const_var("barEmpty", barEmpty);

  ecs.get_const_var("go", go);
  ecs.get_const_var("caution", caution);
  ecs.get_const_var("extraCaution", extraCaution);
  ecs.get_const_var("stop", stop);

  ecs.get_const_var("dryFountain", dryFountain);
  ecs.get_const_var("blood", blood);
  ecs.get_const_var("dung", dung);
  ecs.get_const_var("sensed", sensed);
  ecs.get_const_var("door", door);
  ecs.get_const_var("mappingPath", mappingPath);
}
