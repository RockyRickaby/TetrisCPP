#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_surface.h>
#include <filesystem>

#include "appstate.hpp"
#include "engine/text/fonts.hpp"
#include "engine/texture.hpp"
#include "engine/utils.hpp"
#include "engine/silly3D/silly_3D.hpp"
#include "tetris/tetris_state_machines/tetris_game_sm.hpp"

struct Experimental {
    TEngine::Silly3D::SillyModel* sillymodel;
    TEngine::Silly3D::SillyInstance3D instance;

    TEngine::Silly3D::SillyAssetManager silly_assets{nullptr};
    // TEngine::Silly3D::SillyModel* penger;

    TEngine::Text::BitmapFont fnt;
};

namespace fs = std::filesystem;
using TetrisBag = Tetris::Bag::Standard;

// TODO - improve this to handle MORE events!!
void AppState::raise_event(SDL_Event* event) {
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.repeat) {
            TEngine::Events::KeyRepeatEvent ev{event->key.scancode};
            game_sm->event(ev);
        } else if (event->key.down) {
            TEngine::Events::KeyPressedEvent ev{event->key.scancode};
            game_sm->event(ev);
        }
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
    

    fs::path joust_font_path = fonts_root / "JoustFont";
    // spratlas = TEngine::Sprites::SpriteAtlas(joust_font_path, 8, 8, renderer, SDL_SCALEMODE_NEAREST);
    // font = std::make_unique<Text::JoustFont>(joust_font_path, 8, renderer, TEngine::TUtils::color_from_uint32(0));
    font = Text::BitmapFont::load_font(joust_font_path, 8, renderer, SDL_SCALEMODE_NEAREST, {0,0,0,255});
    // tile size for the game should be about 40 x 24
    mino_texture = TEngine::load_texture(assets_root / "mino.png", SDL_SCALEMODE_LINEAR, renderer);
    game = std::make_unique<Tetris::Game>(
        renderer,
        window_width, window_height,
        static_cast<float>(window_width) / 32.0f,
        &font,
        mino_texture.get()
    );
    menu = std::make_unique<Tetris::MainMenu>(&font, renderer);
    tetris_keys.setup_keys(Tetris::Keybinds::Settings::Default);
    // TODO - implement these main states and set them up properly
    game_sm = std::make_unique<Tetris::States::TetrisStateMachine>(
        game.get(),
        menu.get(),
        renderer,
        &tetris_keys,
        &font
    );

    // game_sm->switch_to(Tetris::States::STATE_MAIN_MENU);
    
    // TODO - make this constructor explicit
    __testing->silly_assets = renderer;
    __testing->sillymodel = &__testing->silly_assets.load_model(
        models_root / "pieces" / "Tpiece.obj"
    );

    // instance = {"penger", sillymodel, {0, 0, 3}, {}, TEngine::TUtils::color_from_hex("#FFDE00"), renderer, width, height};
    __testing->instance = __testing->silly_assets.instance_from(__testing->sillymodel->name, window_width, window_height);
    __testing->instance.position += Vec3{0, 0, 4};
    __testing->instance.rotation = { 0, 0,0 };
    __testing->instance.color = TEngine::TUtils::color_from_hex("#B802FD");
    __testing->instance.cull_wireframe = false;
    __testing->instance.fill = false;
    __testing->instance.jitter = false;

    // __testing->fnt = Text::BitmapFont2::load_font(fonts_root / "JoustFont", 8, renderer, SDL_SCALEMODE_NEAREST);
    return SDL_APP_CONTINUE;
}

SDL_AppResult AppState::update_and_draw() {
    using namespace TEngine;
    double delta = time.delta_time();
    // std::cout << 1/delta << std::endl;
    if (delta >= 0.1) {
        delta = 0.1;
    }
    game_sm->update(delta);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);
    game_sm->draw();

    __testing->instance.rotate_y((3.14f / 2) / 2 * delta);
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

static inline void setup_atlas(TEngine::Sprites::SpriteAtlas* spr) {
    std::vector<std::pair<int, int>> m_char_to_idx{{
        {1, 0},{'a', 0},
        {2, 1},{'b', 1},
        {3, 2},{'c', 2},
        {4, 3},{'d', 3},
        {5, 4},{'e', 4},
        {'F', 5},{'f', 5},
        {'G', 6},{'g', 6},
        {'H', 7},{'h', 7},
        {'I', 8},{'i', 8},
        {'J', 9},{'j', 9},
        {'K', 10},{'k', 10},
        {'L', 11},{'l', 11},
        {'M', 12},{'m', 12},
        {'N', 13},{'n', 13},
        {'O', 14},{'o', 14},
        {'P', 15},{'p', 15},
        {'Q', 16},{'q', 16},
        {'R', 17},{'r', 17},
        {'S', 18},{'s', 18},
        {'T', 19},{'t', 19},
        {'U', 20},{'u', 20},
        {'V', 21},{'v', 21},
        {'W', 22},{'w', 22},
        {'X', 23},{'x', 23},
        {'Y', 24},{'y', 24},
        {'Z', 25},{'z', 25},
        {'0', 26},
        {'1', 27},
        {'2', 28},
        {'3', 29},
        {'4', 30},
        {'5', 31},
        {'6', 32},
        {'7', 33},
        {'8', 34},
        {'9', 35},
        {'?', 36},
        {'(', 39},
        {'-', 40},
        {'.', 41},
        {'=', 44},
        {'!', 45},
        {')', 46},
        {' ', 47} // blank square
    }};

    for (const auto [ch, idx] : m_char_to_idx) {
        spr->insert_offsets(static_cast<int>(ch), idx % 10, idx / 10);
    }
}