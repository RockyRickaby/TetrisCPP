/*
 * :)
 */

#include "engine/events.hpp"
#include "engine/text/fonts.hpp"
#include "engine/text/text_renderer.hpp"
#include "tetris/tetris_bags.hpp"
#include <SDL3/SDL_stdinc.h>
// #include <filesystem>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <memory>

#include "tetris/tetris_game.hpp"
#include "tetris/tetris_menu.hpp"
#include "tetris/tetris_input.hpp"
#include "tetris/tetris_state_machines/tetris_game_sm.hpp"
#include "engine/tengine.hpp"
#include "engine/sprites/sprite_renderer.hpp"
// #include "engine/state_machine.hpp"

using TetrisBag = Tetris::Bag::Standard;

struct AppState {
    /* We will use this renderer to draw into this window every frame. */
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    // TEngine::Text::BitmapRenderer bm_renderer;
    // SDL_Texture *playfield_texture = nullptr;
    // SDL_Texture *bag_texture = nullptr;
    std::unique_ptr<Tetris::Game> game = nullptr; // use new or make it static. size is a bit big
    
    TEngine::Time time;
    Tetris::Keybinds tetris_keys;
    std::unique_ptr<TEngine::Text::BitmapFonts::JoustFont> font;
    // TEngine::StateMachine::GenericStateMachine<int> game_sm;
    std::unique_ptr<Tetris::States::TetrisStateMachine> game_sm;

    void raise_event(SDL_Event* event);
};

TEngine::Text::BitmapText joust_tmp;
std::unique_ptr<TEngine::Sprites::SpriteAtlas> joust_sprites;

class Whatever : public TEngine::Events::ICustomEvent {
public:
    const char* get_name() const { return "Whatever"; }
};

class Whatever3 : public TEngine::Events::ICustomEvent {
public:
    const char* get_name() const { return "Whatever"; }
};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    using namespace TEngine;
    TEngine::Events::CustomEventsDispatcher ced;
    // ced.listen<Whatever>([](auto event) { std::cout << "TRIGGERING WHATEVER  t w o\n"; return true; });
    // ced.listen<Whatever>([](auto event) { std::cout << "TRIGGERING WHATEVER\n"; return true; });
    // ced.listen<Whatever3>([](auto event) { std::cout << "TRIGGERING WHATEVER3\n"; return true; });
    // Whatever wv{};
    // Whatever3 wv2{};
    // Whatever3 wv3{};
    // ced.enqueue(Whatever3{});
    // ced.enqueue(Whatever{});
    // ced.enqueue(Whatever3{});
    // // ced.remove<Whatever>();
    // ced.dispatch(wv);
    // std::cout << "------------\n";
    // ced.dispatch_enqueued();
    // std::cout << argv[0] << std::endl;
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    int width = 800;
    int height = 600;
    AppState *state = new AppState(); // using new operator cuz appstate needs the raw pointer and it has to live until the application quits
    if (!SDL_CreateWindowAndRenderer("Tetris", width, height, SDL_WINDOW_RESIZABLE, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderVSync(state->renderer, true);
    SDL_SetRenderLogicalPresentation(state->renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    state->font = std::make_unique<Text::BitmapFonts::JoustFont>(state->renderer, "assets/fonts/JoustFontAtlas.png");
    // tile size for the game should be about 40 x 24
    state->game = std::make_unique<Tetris::Game>(
        state->renderer,
        width, height,
        static_cast<float>(width) / 32.0f,
        state->font.get()
    );
    // state->game->set_level(13);
    // state->game->set_piece_queue<Tetris::Bag::Random>();
    // state->game = std::make_unique<decltype(state->game)::element_type>(width, height, 20.0f);
    state->tetris_keys.setup_keys(Tetris::Keybinds::Settings::Default);
    state->game_sm = std::make_unique<Tetris::States::TetrisStateMachine>(
        state->game.get(),
        state->renderer,
        &state->tetris_keys,
        state->font.get()
    );
    // TODO - implement these main states and set them up properly
    // state->game_sm.add_state<Tetris::States::RunGameState>(Tetris::States::STATE_TETRIS, &state->game_sm, state->game.get(), state->renderer, &state->tetris_state_input, &state->tetris_state_input)
    //         .add_state<Tetris::States::MenuState>(Tetris::States::STATE_MAIN_MENU, &state->game_sm, state->renderer, &state->tetris_state_input)
    //         .add_state<Tetris::States::GameOverState>(Tetris::States::STATE_GAMEOVER, &state->game_sm, state->renderer, &state->tetris_state_input, state->font.get())
    //         .switch_to(Tetris::States::STATE_MAIN_MENU);
    // state->game_sm.add_state("rungame", std::move(state_run));
    // TetrisUtils::color_from_hex("#00E6FE");

    // std::cout << std::filesystem::current_path() << std::endl;
    joust_tmp = Text::BitmapFontRenderer::make_bitmap_text_num(state->font.get(), 0);
    joust_sprites = std::make_unique<TEngine::Sprites::SpriteAtlas>("assets/fonts/JoustFontAtlas.png", 8, 8, state->renderer, SDL_SCALEMODE_NEAREST);
    joust_sprites->insert_offsets(1, 16, 4);
    *appstate = state;

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

// useful for instant actions
/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *state = static_cast<AppState*>(appstate);
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE || event->key.key == SDLK_Q) {
            return SDL_APP_SUCCESS;
        }
        // state->tetris_state_input.read_input_event(event->key);
    }
    state->raise_event(event);

    // if (event->type == SDL_EVENT_KEY_DOWN) {
        // state->tetris_state_input.read_input_event(event->key);
    // }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

// useful for continuous actions, like moving on a plane/space
/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    using namespace TEngine;
    AppState *state = static_cast<AppState*>(appstate);
    double delta = state->time.delta_time();
    if (delta >= 0.1) {
        delta = 0.1;
    }
    // do_input_working(state);
    // do_input_events(state);
    // do_input_state(state);
    // state->game->update(delta);
    // std::cout << 1/delta << std::endl;

    // state->game_sm->handle_input();

    // std::cout << "trying\n";
    // state->game_sm->handle_input_state(state->tetris_state_input);
    state->game_sm->update(delta);
    
    // SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    // SDL_RenderClear(state->renderer);
    // state->game->draw(state->renderer);
    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(state->renderer);
    state->game_sm->draw();

    // Tetris::Vec2 vec;
    // Uint32 buttons = SDL_GetMouseState(&vec.x, &vec.y);
    // std::cout << vec << std::endl;
    /* put the newly-cleared rendering on the screen. */
    
    // SDL_RenderTexture(state->renderer, joust_tmp.text_data, nullptr, &corner);
    // SDL_FRect corner = {10,10,static_cast<float>(joust_tmp.text_data->w) * 1,static_cast<float>(joust_tmp.text_data->h) * 1};
    // Text::draw_bitmap_text(joust_tmp, corner);
    // BitmapFontRenderer::draw_string(state->font.get(), "HOLD\n  ON", 320, 240, 10);
    // Text::BitmapFontRenderer::render_char(state->font.get(), 'H', 320, 240, 3);
    Sprites::SpriteRenderer::draw_from_atlas(joust_sprites.get(), 1, 0, 0, 10);
    SDL_RenderPresent(state->renderer);
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    using namespace TEngine;
    /* SDL will clean up the window/renderer for us. */
    AppState *state = static_cast<AppState*>(appstate);
    // SDL_SetRenderVSync(state->renderer, true);
    // SDL_DestroyTexture(state->playfield_texture);
    // SDL_DestroyTexture(state->bag_texture);
    Text::destroy_bitmap_text(joust_tmp);
    // TEngine::Text::destroy_bitmap_renderer(state->bm_renderer);
    delete state; // state->game will be destroyed here
}

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