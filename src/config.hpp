#pragma once

#include <flecs.h>

#include <filesystem>
#include <string>

struct Config {
  Config(flecs::world ecs) {
    ecs.module<Config>("Config");
    ecs.component<std::string>()
        .opaque(flecs::String) // Opaque type that maps to string
        .serialize([](const flecs::serializer *s, const std::string *data) {
          const char *str = data->c_str();
          return s->value(flecs::String, &str); // Forward to serializer
        })
        .assign_string([](std::string *data, const char *value) {
          *data = value; // Assign new value to std::string
        });
    auto script = std::filesystem::path("assets/config.flecs");
    ecs.script().filename(script.string().c_str()).run();

    get_const_var(ecs, "title", title);
    get_const_var(ecs, "font", font);
    get_const_var(ecs, "cover_image", cover_image);

    get_const_var(ecs, "music", music);
    get_const_var(ecs, "drone", drone);
  }

  static inline void get_const_var(flecs::world ecs, const char *name,
                                   std::string &var) {
    auto id = ecs_lookup(ecs, (std::string("Config.") + name).c_str());
    auto ptr = ecs_const_var_get(ecs, id);
    var = *(char **)ptr.ptr;
  }

  static inline std::string title;
  static inline std::string font;
  static inline std::string cover_image;

  static inline std::string music;
  static inline std::string drone;
};
