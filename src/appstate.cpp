#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_rect.h>
#include <filesystem>
#include <cassert>
#include <array>
#include <memory>
#include <print>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "appstate.hpp"
#include "engine/events.hpp"
#include "engine/shapes/bezier.hpp"
#include "engine/shapes/hermite.hpp"
#include "engine/shapes/splines.hpp"
#include "engine/sound/audio.hpp"
#include "engine/shapes/shapes.hpp"
#include "engine/tengine.hpp"
#include "engine/utils.hpp"
#include "engine/text/fonts.hpp"
#include "engine/text/text_renderer.hpp"
#include "engine/texture.hpp"
#include "engine/silly3D/silly_3D.hpp"
#include "tetris/tetris_piece3d.hpp"
#include "tetris/tetris_state_machines/tetris_game_sm.hpp"

struct Experimental {
    TEngine::Silly3D::SillyModel* sillymodel;
    TEngine::Silly3D::SillyInstance3D instance;
    TEngine::Silly3D::SillyInstance3D instance2;

    TEngine::Text::BitmapFont* fnt;
};

class Testing : public TEngine::Events::IEventListener {
public:
    bool OnKeyPressed(TEngine::Events::KeyPressedEvent&) override { std::println("it worked"); return false; }
    bool OnKeyReleased(TEngine::Events::KeyReleasedEvent&) override { std::println("it worked"); return false; }
    bool OnKeyRepeat(TEngine::Events::KeyRepeatEvent&) override { std::println("it worked"); return false; }
    bool OnApplicationExit(TEngine::Events::ApplicationExitEvent&) override { std::println("it worked"); return false; }
};

namespace fs = std::filesystem;
using TetrisBag = Tetris::Bag::Standard;

TEngine::Text::BitmapText test;

// static constexpr TEngine::Splines::Bezier2D::CubicBezier curve{{0,0}, {0.42 * 960,0.0 * 720}, {0.58 * 960,1.0 * 720}, {1 * 960,1 * 720}};
// static float t_bz = 0;

SDL_AppResult AppState::setup_app() {
    using namespace TEngine;
    metadata = {
        "Tetris C++",
        "0.0",
        "com.mauro.tetriscpp"
    };

    window = {
        .window_name = "Tetris C++",
        .width = 960,
        .height = 720,
        .window_flags = SDL_WINDOW_RESIZABLE,
        .logical_width = 960,
        .logical_height = 720,
        .logical_window_mode = SDL_LOGICAL_PRESENTATION_LETTERBOX,
        .use_logical_window_size = true,
        .vsync = true
    };
    if (!(TEngine::init(metadata) && TEngine::init_window(window))) {
        return SDL_APP_FAILURE;
    }

    __testing = new Experimental();
    silly_assets3d.set_renderer(window.renderer);
    using namespace Tetris::Tetrimino;
    const auto v = std::array{
        Type::I,
        Type::J,
        Type::L,
        Type::O,
        Type::S,
        Type::Z,
        Type::T,
    };
    
    for (const auto type : v) {
        pieces3dfill.emplace_back(Tetris::load_piece3d_tri(pieces_root, silly_assets3d, type));
        auto& piece_fill = pieces3dfill.back();
        piece_fill.instance.update_window(window.logical_width, window.logical_height);
        piece_fill.reset();
        
        pieces3dwire.emplace_back(Tetris::load_piece3d_quad(pieces_root, silly_assets3d, type));
        auto& piece_wire = pieces3dwire.back();
        piece_wire.instance.update_window(window.logical_width, window.logical_height);
        piece_wire.reset();
    }

    fs::path joust_font_path = fonts_root / "JoustFont";
    font = Text::BitmapFont::load_font(joust_font_path, 8, window.renderer, SDL_SCALEMODE_NEAREST, {0,0,0,255});
    // tile size for the game should be about 40 x 24
    mino_texture = TEngine::load_texture(assets_root / "mino.png", SDL_SCALEMODE_LINEAR, window.renderer);
    load_sounds();

    tetris_keys.setup_keys(Tetris::Keybinds::Settings::Default);
    game = std::make_unique<Tetris::Game>(
        window.renderer,
        window.logical_width, window.logical_height,
        static_cast<float>(window.logical_width) / 32.0f,
        &font,
        mino_texture.get(),
        &mixer
    );
    menu = std::make_unique<Tetris::MainMenu>(&font, &tetris_keys, window.renderer, pieces3dwire, pieces3dfill);
    // TODO - implement these main states and set them up properly
    game_sm = std::make_unique<Tetris::States::TetrisStateMachine>(
        game.get(),
        menu.get(),
        window.renderer,
        &tetris_keys,
        &font,
        leaderboard,
        pieces3dwire,
        pieces3dfill,
        SDL_GetWindowID(window.window),
        tracks_from_music
    );

    // game_sm->switch_to(Tetris::States::STATE_GAMEOVER);

    __testing->fnt = &font;
    test = Text::BitmapFontRenderer::make_bitmap_text_str(&font, "tetris\n   100");
    return SDL_APP_CONTINUE;
}

AppState::~AppState() {
    // SDL_StopTextInput(window.window);
    tracks_from_music.clear();
    tracks_sfx.clear();
    music.clear();
    sfx.clear();
    mixer.reset(); // needed to prevent segfault when calling MIX_DestroyMixer AFTER MIX_Quit;

    MIX_Quit();
    TEngine::end_window(window);
    delete __testing;
}

void AppState::load_sounds() {
    using namespace TEngine;
    constexpr std::array sfx_files = {
        "piece_lock.ogg",
        "line_clear1.ogg",
        "line_clear2.ogg",
        "line_clear3.ogg",
        "gameover.ogg",
        "4line_clear.ogg",
    };
    // TROIKA = 3s 280ms
    // LOGINSKA = 30s 73ms
    constexpr std::array music_files = {
        std::make_pair("loginska.ogg", "loginska"),
        std::make_pair("troika.ogg", "troika"),
        std::make_pair("kalinka.ogg", "kalinka"),
        std::make_pair("3.ogg", "3"),
    };
    
    MIX_Init();
    SDL_AudioSpec spec = {
        .format = SDL_AUDIO_S16,
        .channels = 2,
        .freq = 48000
    };
    mixer = Sound::Mixer{MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec)};

    for (const std::string_view filename : sfx_files) {
        if (!fs::exists(sounds_root / filename)) {
            std::println(stderr, "sfx file does not exist: {}", filename);
            continue;
        }
        auto ptr_audio = MIX_LoadAudio(mixer.get(), (sounds_root / filename).string().c_str(), true);
        if (!ptr_audio) {
            continue;
        }
        sfx.emplace_back(ptr_audio);

        auto ptr_track = MIX_CreateTrack(mixer.get());
        if (!ptr_track) {
            music.pop_back();
            continue;
        }

        MIX_SetTrackAudio(ptr_track, ptr_audio);
        MIX_TagTrack(ptr_track, "game_sounds");
        MIX_TagTrack(ptr_track, std::string{filename.substr(0, filename.find_last_of('.'))}.c_str());

        tracks_sfx.emplace_back(ptr_track, mixer);
    }

    for (const auto& [filename, song_name] : music_files) {
        if (!fs::exists(sounds_root / filename)) {
            std::println(stderr, "music file does not exist: {}", filename);
            continue;
        }
        auto ptr_audio = MIX_LoadAudio(mixer.get(), (sounds_root / filename).string().c_str(), false);
        if (!ptr_audio) {
            continue;
        }
        music.emplace_back(ptr_audio);
        auto ptr_track = MIX_CreateTrack(mixer.get());
        if (!ptr_track) {
            music.pop_back();
            continue;
        }

        MIX_SetTrackAudio(ptr_track, ptr_audio);
        MIX_TagTrack(ptr_track, song_name);
        MIX_TagTrack(ptr_track, "game_sounds");
        Sound::Track song_track{ptr_track, mixer};

        std::string_view name_strview{song_name};
        song_track.m_properties = SDL_CreateProperties();

        SDL_SetNumberProperty(song_track.m_properties, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
        if (name_strview == "loginska") {
            SDL_SetNumberProperty(song_track.m_properties, MIX_PROP_PLAY_LOOP_START_MILLISECOND_NUMBER, 30073);
        } else if (name_strview == "troika") {
            SDL_SetNumberProperty(song_track.m_properties, MIX_PROP_PLAY_LOOP_START_MILLISECOND_NUMBER, 3279);
        }
        tracks_from_music.emplace_back(std::move(song_track));
    }
    // MIX_SetTagGain(mixer.get(), "game_sounds", 0.25f);
    MIX_SetMixerGain(mixer.get(), 0.25f);
}


SDL_AppResult AppState::update_and_draw() {
    using namespace TEngine;
    double delta = time.delta_time();
    // std::cout << 1/delta << std::endl;
    if (delta >= 0.1) {
        delta = 0.1;
    }
    game_sm->update(delta);
    
    SDL_SetRenderDrawColor(window.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(window.renderer);
    game_sm->draw();

    // auto v = TEngine::Splines::spline2d(curve, t_bz);
    // // v.y *= 720;
    // // v.x *= 960;
    // // v.y = 720 - v.y;
    // t_bz += 0.25 * delta;
    // if (t_bz >= 1) {
    //     t_bz = 0;
    // }
    // SDL_FRect r{v.x - 7.5f, v.y - 7.5f, 15, 15};
    // SDL_RenderFillRect(window.renderer, &r);

    Splines::spline2d<Splines::Bezier2D::QuadraticBezier>({}, 1);
    // Shape::draw_spline2d(window.renderer, curve, 16);
    SDL_RenderPresent(window.renderer);
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void AppState::raise_event(SDL_Event* event) {
    using namespace TEngine::Events;
    Tetris::States::TetrisStateMachine& recv = *(game_sm.get());
    switch (event->type) {
        case SDL_EVENT_QUIT: {
            forward_event<ApplicationExitEvent>(recv);
        } break;
        case SDL_EVENT_TEXT_INPUT: {
            forward_event<TextInputEvent>(recv, event->text.text);
        } break;
        case SDL_EVENT_TEXT_EDITING: {
            forward_event<TextEditingEvent>(recv,
                event->edit.text,
                event->edit.start,
                event->edit.length
            );
        } break;
        case SDL_EVENT_KEY_DOWN: {
            if (event->key.repeat) {
                forward_event<KeyRepeatEvent>(recv, event->key.scancode);
            } else if (event->key.down) {
                forward_event<KeyPressedEvent>(recv, event->key.scancode);
            }
        } break;
        case SDL_EVENT_KEY_UP: {
            forward_event<KeyReleasedEvent>(recv, event->key.scancode);
        } break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            forward_event<MousePressedEvent>(recv,
                event->button.button,
                event->button.x,
                event->button.y,
                event->button.clicks
            );
        } break;
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            forward_event<MouseReleasedEvent>(recv,
                event->button.button,
                event->button.x,
                event->button.y
            );
        } break;
        case SDL_EVENT_MOUSE_MOTION: {
            forward_event<MouseMovedEvent>(recv,
                event->motion.x,
                event->motion.y,
                event->motion.xrel,
                event->motion.yrel    
            );
        } break;
        case SDL_EVENT_MOUSE_WHEEL: {
            forward_event<MouseWheelEvent>(recv,
                event->wheel.x,
                event->wheel.y,
                event->wheel.mouse_x,
                event->wheel.mouse_y,
                event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED
            );
        } break;
        case SDL_EVENT_WINDOW_RESIZED: {
            forward_event<WindowResizedEvent>(recv,
                event->window.data1,
                event->window.data2
            );
        } break;

        default: break;
    }
}