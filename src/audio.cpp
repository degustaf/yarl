#include "audio.hpp"
// #include "corruption.hpp"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <cassert>
#include <cstdio>

SDLAudio::SDLAudio(const std::filesystem::path &music_path,
                   const std::filesystem::path &drone_path)
    : mixer(nullptr, nullptr), music_track(nullptr, nullptr),
      drone_track(nullptr, nullptr) {
  if (!MIX_Init()) {
    SDL_Log("Unable to initialize SDL Mixer: %s\n", SDL_GetError());
    return;
  }

  mixer = MixerPtr(
      MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr),
      MIX_DestroyMixer);
  if (!mixer) {
    SDL_Log("Unable to create mixer: %s\n", SDL_GetError());
    return;
  }

  music_track = TrackPtr(MIX_CreateTrack(mixer.get()), MIX_DestroyTrack);
  if (!music_track) {
    SDL_Log("Couldn't create track: %s\n", SDL_GetError());
    return;
  }
  auto music = MIX_LoadAudio(mixer.get(), music_path.string().c_str(), true);
  if (!music) {
    SDL_Log("Couldn't load audio from %s: %s\n", music_path.string().c_str(),
            SDL_GetError());
    return;
  }
  if (!MIX_SetTrackAudio(music_track.get(), music)) {
    SDL_Log("Couldn't set track audio: %s\n", SDL_GetError());
    return;
  }
  MIX_DestroyAudio(music);

  drone_track = TrackPtr(MIX_CreateTrack(mixer.get()), MIX_DestroyTrack);
  if (!drone_track) {
    SDL_Log("Couldn't create track: %s\n", SDL_GetError());
    return;
  }
  auto drone = MIX_LoadAudio(mixer.get(), drone_path.string().c_str(), true);
  if (!drone) {
    SDL_Log("Couldn't load audio from %s: %s\n", drone_path.string().c_str(),
            SDL_GetError());
    return;
  }
  if (!MIX_SetTrackAudio(drone_track.get(), drone)) {
    SDL_Log("Couldn't set track audio: %s\n", SDL_GetError());
    return;
  }
  MIX_DestroyAudio(drone);

  set_track_mix(0.0f);

  auto props = SDL_CreateProperties();
  SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
  if (!MIX_PlayTrack(music_track.get(), props)) {
    SDL_Log("Couldn't start track playback: %s\n", SDL_GetError());
    return;
  }
  if (!MIX_PlayTrack(drone_track.get(), props)) {
    SDL_Log("Couldn't start track playback: %s\n", SDL_GetError());
    return;
  }
  SDL_DestroyProperties(props);
}

void SDLAudio::set_track_mix(float f) {
  mix = f;
  MIX_SetTrackGain(drone_track.get(), mix * music_gain);
  MIX_SetTrackGain(music_track.get(), (1.0f - mix) * music_gain);
}

void SDLAudio::set_music_gain(float f) {
  music_gain = f;
  MIX_SetTrackGain(drone_track.get(), mix * music_gain);
  MIX_SetTrackGain(music_track.get(), (1.0f - mix) * music_gain);
}

// void AudioCorruptionObserver::onNotify(flecs::entity e, Event event) {
//   switch (event) {
//   case Event::CorruptionChanged: {
//     auto f = e.has<Corruption>() ? e.get<Corruption>().ratio() : 0.0f;
//     e.world().get_mut<SDLAudio>().set_track_mix(f);
//     break;
//   }
//   default:
//     break;
//   }
// }
