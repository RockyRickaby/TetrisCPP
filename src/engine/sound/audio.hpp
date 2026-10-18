#pragma once

#include <memory>
// #include <filesystem>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

namespace TEngine::Sound {
    struct __AudioDestructor {
        void operator()(MIX_Audio* a) {
            MIX_DestroyAudio(a);
        }
    };
    struct __MixerDestructor {
        void operator()(MIX_Mixer* a) {
            MIX_DestroyMixer(a);
        }
    };
    struct __TrackDestructor {
        void operator()(MIX_Track* a) {
            MIX_DestroyTrack(a);
        }
    };

    using Audio = std::unique_ptr<MIX_Audio, __AudioDestructor>;
    using Mixer = std::unique_ptr<MIX_Mixer, __MixerDestructor>;

    class Track;
    // plays once and doesn't stop (one-shot)
    // using Audio = std::unique_ptr<MIX_Audio, __AudioDestructor>;
    // mixers load tracks and audio (they also play audio, while tracks can play themselves)
    // maybe these should be classes instead so that we can just call track.play() and track.pause()
    // but I also want to avoid having too much object orientation...
    // using Mixer = std::unique_ptr<MIX_Mixer, __MixerDestructor>;
    class Track : public std::unique_ptr<MIX_Track, __TrackDestructor> {
    public:
        Track() :
            unique_ptr{},
            m_mixer{nullptr}
        {} 
        Track(MIX_Track* track_ptr, Mixer& mixer_ptr) :
            unique_ptr{track_ptr},
            m_mixer{&mixer_ptr}
        {}

        Mixer* m_mixer;
        SDL_PropertiesID m_properties;
    };

    // these are currently not ready to use
    class SharedAudio : public std::shared_ptr<MIX_Audio> {
    public:
        SharedAudio() : shared_ptr{} {}
        SharedAudio(MIX_Audio* ptr) : shared_ptr{ptr, __AudioDestructor{}} {}
    };

    class SharedMixer : public std::shared_ptr<MIX_Mixer> {
    public:
        SharedMixer() : shared_ptr{} {}
        SharedMixer(MIX_Mixer* ptr) : shared_ptr{ptr, __MixerDestructor{}} {}
    };

    class SharedTrack : public std::shared_ptr<MIX_Track> {
    public:
        SharedTrack() : shared_ptr{} {}
        SharedTrack(MIX_Track* ptr) : shared_ptr{ptr, __TrackDestructor{}} {}

    private:
        MIX_Mixer* m_mixer;
    };
}