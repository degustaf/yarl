#pragma once

#include <SDL3_mixer/SDL_mixer.h>

#include <assert.h>
#include <filesystem>
#include <memory>

// #include "observer.hpp"

using MixerPtr = std::unique_ptr<MIX_Mixer, decltype(&MIX_DestroyMixer)>;
using AudioPtr = std::unique_ptr<MIX_Audio, decltype(&MIX_DestroyAudio)>;
using TrackPtr = std::unique_ptr<MIX_Track, decltype(&MIX_DestroyTrack)>;

struct SDLAudio {
public:
  SDLAudio() = delete;
  SDLAudio(const std::filesystem::path &music_path,
           const std::filesystem::path &drone_path);

  void set_track_mix(float f);
  inline void set_master_gain(float f) {
    assert(f >= 0.0f);
    MIX_SetMixerGain(mixer.get(), f);
  }
  void set_music_gain(float f);
  inline void set_sfx_gain(float f) { sfx_gain = f; }
  inline float get_master_gain() const { return MIX_GetMixerGain(mixer.get()); }
  inline float get_music_gain() const { return music_gain; }
  inline float get_sfx_gain() const { return sfx_gain; }
  inline void add_master_gain(float f) {
    set_master_gain(get_master_gain() + f);
  }
  inline void add_music_gain(float f) { set_music_gain(get_music_gain() + f); }
  inline void add_sfx_gain(float f) { set_sfx_gain(get_sfx_gain() + f); }

private:
  MixerPtr mixer;
  TrackPtr music_track;
  TrackPtr drone_track;
  float mix = 0.0f;
  float music_gain = 1.0f;
  float sfx_gain = 1.0f;
};

// struct AudioCorruptionObserver : Observer {
//   virtual ~AudioCorruptionObserver() = default;
//   virtual void onNotify(flecs::entity, Event);
// };
