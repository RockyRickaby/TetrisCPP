#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <filesystem>
#include <array>
#include <utility>

#include "appstate.hpp"
#include "engine/events.hpp"
#include "engine/text/fonts.hpp"
#include "engine/texture.hpp"
#include "engine/utils.hpp"
#include "engine/silly3D/silly_3D.hpp"
#include "tetris/tetris_state_machines/tetris_game_sm.hpp"

struct Experimental {
    TEngine::Silly3D::SillyModel* sillymodel;
    TEngine::Silly3D::SillyInstance3D instance;

    // TEngine::Silly3D::SillyModel* penger;

    TEngine::Text::BitmapFont fnt;
};

namespace fs = std::filesystem;
using TetrisBag = Tetris::Bag::Standard;

void AppState::raise_event(SDL_Event* event) {
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.repeat) {
            TEngine::Events::KeyRepeatEvent ev{event->key.scancode};
            game_sm->event(ev);
        } else if (event->key.down) {
            TEngine::Events::KeyPressedEvent ev{event->key.scancode};
            game_sm->event(ev);
        }
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        TEngine::Events::MousePressedEvent ev{
            event->button.button,
            event->button.x,
            event->button.y,
            event->button.clicks
        };
        game_sm->event(ev);
    } else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        TEngine::Events::MouseReleasedEvent ev{
            event->button.button,
            event->button.x,
            event->button.y
        };
        game_sm->event(ev);
    } else if (event->type == SDL_EVENT_MOUSE_MOTION) {
        TEngine::Events::MouseMovedEvent ev{
            event->motion.x,
            event->motion.y,
            event->motion.xrel,
            event->motion.yrel
        };
        game_sm->event(ev);
    } else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        TEngine::Events::MouseWheelEvent ev{
            event->wheel.x,
            event->wheel.y,
            event->wheel.mouse_x,
            event->wheel.mouse_y,
            event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED
        };
        game_sm->event(ev);
    }
}

SDL_AppResult AppState::setup_app() {
    using namespace TEngine;
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    window_width = 960;
    window_height = 720;
    if (!SDL_CreateWindowAndRenderer("Tetris", window_width, window_height, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    // SDL_SetRenderVSync(renderer, true);
    SDL_SetRenderLogicalPresentation(renderer, window_width, window_height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    __testing = new Experimental();
    silly_assets3d = Silly3D::SillyAssetManager{renderer};

    fs::path joust_font_path = fonts_root / "JoustFont";
    // spratlas = TEngine::Sprites::SpriteAtlas(joust_font_path, 8, 8, renderer, SDL_SCALEMODE_NEAREST);
    // font = std::make_unique<Text::JoustFont>(joust_font_path, 8, renderer, TEngine::TUtils::color_from_uint32(0));
    font = Text::BitmapFont::load_font(joust_font_path, 8, renderer, SDL_SCALEMODE_NEAREST, {0,0,0,255});
    // tile size for the game should be about 40 x 24
    mino_texture = TEngine::load_texture(assets_root / "mino.png", SDL_SCALEMODE_LINEAR, renderer);

    tetris_keys.setup_keys(Tetris::Keybinds::Settings::Default);
    game = std::make_unique<Tetris::Game>(
        renderer,
        window_width, window_height,
        static_cast<float>(window_width) / 32.0f,
        &font,
        mino_texture.get()
    );
    menu = std::make_unique<Tetris::MainMenu>(&font, &tetris_keys, renderer);
    // TODO - implement these main states and set them up properly
    game_sm = std::make_unique<Tetris::States::TetrisStateMachine>(
        game.get(),
        menu.get(),
        renderer,
        &tetris_keys,
        &font
    );

    using namespace Tetris::Tetrimino;
    const auto v = std::array{
        std::make_pair<std::string, Type>("Ipiece", Type::I),
        std::make_pair<std::string, Type>("Jpiece", Type::J),
        std::make_pair<std::string, Type>("Lpiece", Type::L),
        std::make_pair<std::string, Type>("Opiece", Type::O),
        std::make_pair<std::string, Type>("Spiece", Type::S),
        std::make_pair<std::string, Type>("Tpiece", Type::T),
        std::make_pair<std::string, Type>("Zpiece", Type::Z)
    };

    for (const auto& [file, type] : v) {
        // [[maybe_unused]] auto& m1 = asset
        const auto& m1 = silly_assets3d.load_model(models_root / "pieces" / (file + "Tri.obj"));
        pieces3dfill.emplace_back(silly_assets3d.instance_from(m1.name, window_width, window_height));
        pieces3dfill.back().fill = true;
        pieces3dfill.back().position = {0,0,4};
        pieces3dfill.back().color = Tetris::Tetrimino::get_piece(type).get_color();

        const auto& m2 = silly_assets3d.load_model(models_root / "pieces" / (file + ".obj"));
        pieces3dwire.emplace_back(silly_assets3d.instance_from(m2.name, window_width, window_height));
        pieces3dwire.back().cull_wireframe = true;
        pieces3dwire.back().position = {0,0,4};
        pieces3dwire.back().color = pieces3dfill.back().color;
    }

    // game_sm->switch_to(Tetris::States::STATE_MAIN_MENU);
    
    // TODO - make this constructor explicit
    __testing->sillymodel = &silly_assets3d.load_model(
        models_root / "old" / "penger.obj"
    );

    // instance = {"penger", sillymodel, {0, 0, 3}, {}, TEngine::TUtils::color_from_hex("#FFDE00"), renderer, width, height};
    __testing->instance = silly_assets3d.instance_from(__testing->sillymodel->name, window_width, window_height);
    __testing->instance.position += Vec3{0, 0, 4};
    __testing->instance.rotation = { 0, 0,0 };
    __testing->instance.color = TEngine::TUtils::color_from_hex("#B802FD");
    __testing->instance.cull_wireframe = false;
    __testing->instance.fill = true;
    __testing->instance.jitter = false;

    // __testing->fnt = Text::BitmapFont2::load_font(fonts_root / "JoustFont", 8, renderer, SDL_SCALEMODE_NEAREST);
    return SDL_APP_CONTINUE;
}

// static double lim = 1.0/(60);
static double lim = 0;
static double acc = 0;
SDL_AppResult AppState::update_and_draw() {
    using namespace TEngine;
    double delta = time.delta_time();
    std::cout << 1/delta << std::endl;
    if (delta >= 0.1) {
        delta = 0.1;
    }
    game_sm->update(delta);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    game_sm->draw();

    for (auto& m : pieces3dfill) {
        if (m.model->name != "TpieceTri") continue;
        m.rotate_y((1.0f / 2) / 2 * delta);
        // m.draw_geometry({}, 0.5f);
        m.draw_wireframe();
        // break;
    }

    acc += delta;
    if (acc >= lim) {
        __testing->instance.rotate_y((1.0f / 2.0f) / 2.0f * acc);
        acc = 0;
    }
    __testing->instance.draw_instance({0,100,-100}, .5f);

    // __testing->fnt.render_char('c', 5, 5, 5);
    // __testing->instance.draw_wireframe();

    /* put the newly-cleared rendering on the screen. */    
    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

AppState::~AppState() {
    delete __testing;
}