#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>
#include <filesystem>
#include <array>
#include <memory>

#include "appstate.hpp"
#include "engine/events.hpp"
#include "engine/tengine.hpp"
#include "engine/text/fonts.hpp"
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

namespace fs = std::filesystem;
using TetrisBag = Tetris::Bag::Standard;

SDL_AppResult AppState::setup_app() {
    using namespace TEngine;
    metadata = {
        "Example Renderer Clear",
        "1.0",
        "com.example.renderer-clear"
    };

    window = {
        .window_name = "main_window",
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
    // TODO - specialize formatters for the types in TEngine
    // std::cout << std::format("Color: {}", TEngine::Color{0,0,0}) << std::endl;
    // static_assert(std::is_default_constructible_v<TEngine::Color>);
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
    // spratlas = TEngine::Sprites::SpriteAtlas(joust_font_path, 8, 8, window.renderer, SDL_SCALEMODE_NEAREST);
    // font = std::make_unique<Text::JoustFont>(joust_font_path, 8, window.renderer, TEngine::TUtils::color_from_uint32(0));
    font = Text::BitmapFont::load_font(joust_font_path, 8, window.renderer, SDL_SCALEMODE_NEAREST, {0,0,0,255});
    // tile size for the game should be about 40 x 24
    mino_texture = TEngine::load_texture(assets_root / "mino.png", SDL_SCALEMODE_LINEAR, window.renderer);

    tetris_keys.setup_keys(Tetris::Keybinds::Settings::Default);
    game = std::make_unique<Tetris::Game>(
        window.renderer,
        window.logical_width, window.logical_height,
        static_cast<float>(window.logical_width) / 32.0f,
        &font,
        mino_texture.get()
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
        SDL_GetWindowID(window.window)
    );

    // game_sm->switch_to(Tetris::States::STATE_GAMEOVER);
    
    // TODO - make this constructor explicit
    // __testing->sillymodel = &silly_assets3d.load_model(
    //     models_root / "pieces" / "Tpiece.obj"
    // );

    // // instance = {"penger", sillymodel, {0, 0, 3}, {}, TEngine::TUtils::color_from_hex("#FFDE00"), window.renderer, width, height};
    // __testing->instance = silly_assets3d.instance_from(__testing->sillymodel->name, window.width, window.height);
    // __testing->instance.position += Vec3{0, 0, 4};
    // // __testing->instance.rotation = { -SDL_PI_F/2, 0,0 };
    // __testing->instance.color = TEngine::TUtils::color_from_hex("#B802FD");
    // __testing->instance.cull_wireframe = false;
    // __testing->instance.fill = false;
    // __testing->instance.jitter = false;

    // __testing->instance2 = __testing->instance;
    // __testing->instance2.position = Vec3{-2, 0, 4};
    __testing->fnt = &font;
    return SDL_APP_CONTINUE;
}

AppState::~AppState() {
    // SDL_StopTextInput(window.window);
    TEngine::end_window(window);
    delete __testing;
}


static double lim = 1.0/(20);
// static double lim = 0;
static double acc = 0;
static double ang = 0;
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

    // for (auto& m : pieces3dfill) {
    //     if (m.model->name != "TpieceTri") continue;
    //     m.rotate_y((1.0f / 2) / 2 * delta);
    //     // m.draw_geometry({}, 0.5f);
    //     m.draw_wireframe();
    //     // break;
    // }

    acc += delta;
    // Text::BitmapFontRenderer::draw_string_line(&__testing->fnt, "tetris", 0, 0, 10, ang, Text::TextFlipMode::None);
    // Text::BitmapFontRenderer::draw_string_line(&__testing->fnt, "hold", 0, 0, static_cast<float>(window.width) / 32.0f / 11.5f, 0, Text::TextFlipMode::None);
    // Color old = Text::BitmapFontRenderer::set_text_color_mod(&__testing->fnt, TEngine::TUtils::color_from_hex("#FF0000"));
    // Text::BitmapFontRenderer::draw_int64(&__testing->fnt, 1234, 0, 0, 10, 0);
    // Text::BitmapFontRenderer::set_text_color_mod(&__testing->fnt, old);
    ang += ((SDL_PI_F / 2.0f) * delta);
    if (acc >= lim) {
        __testing->instance.rotate_y((SDL_PI_F / 2.0f) * acc);
        acc = 0;
    }
    __testing->instance2.rotate_y((SDL_PI_F / 2.0f) * delta);

    // __testing->instance.draw_instance({0,100,-100}, .5f);
    // __testing->instance2.draw_instance({0,100,-100}, .5f);

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(window.renderer);
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void AppState::raise_event(SDL_Event* event) {
    using namespace TEngine::Events;
    Tetris::States::TetrisStateMachine& recv = *(game_sm.get());

    switch (event->type) {
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