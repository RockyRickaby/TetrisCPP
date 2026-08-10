/*
 * :)
 */

#include "engine/text/fonts.hpp"
#include "engine/text/text_renderer.hpp"
#include "tetris/tetris_bags.hpp"
#include <filesystem>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <memory>

#include "tetris/tetris_game.hpp"
#include "tetris/tetris_state_machines/tetris_game_sm.hpp"
#include "engine/tengine.hpp"
#include "engine/state_machine.hpp"

using TetrisBag = Tetris::Bag::Standard;

struct AppState {
    /* We will use this renderer to draw into this window every frame. */
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    TEngine::Text::BitmapRenderer* bm_renderer;
    // SDL_Texture *playfield_texture = nullptr;
    // SDL_Texture *bag_texture = nullptr;
    std::unique_ptr<Tetris::Game> game = nullptr; // use new or make it static. size is a bit big
    
    TEngine::GameInput game_input;
    TEngine::Time time;

    TEngine::StateMachine::GenericStateMachine<int> game_sm;
};

TEngine::Text::BitmapText joust_tmp{nullptr, nullptr};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    // std::cout << argv[0] << std::endl;
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    int width = 640;
    int height = 480;
    AppState *state = new AppState(); // using new operator cuz appstate needs the raw pointer and it has to live until the application quits
    if (!SDL_CreateWindowAndRenderer("Tetris", width, height, SDL_WINDOW_RESIZABLE, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderVSync(state->renderer, true);
    SDL_SetRenderLogicalPresentation(state->renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    state->bm_renderer = TEngine::Text::init_bitmap_renderer(state->renderer);
    // state->playfield_texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640 * 2, 480 * 2);
    // state->bag_texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 400, 200);
    // sizeof(Tetris::Game<TetrisBag>);

    state->game = std::make_unique<Tetris::Game>(width, height, 20.0f);
    // state->game->set_level(13);
    // state->game->set_piece_queue<Tetris::Bag::Random>();
    // state->game = std::make_unique<decltype(state->game)::element_type>(width, height, 20.0f);
    state->game_input.setup_keys(TEngine::GameInput::Keybinds::KEYBIND_DEFAULT);
    // state->game->init(state->playfield_texture);
    // state->game->init();
    // state->time = {};
    
    // TODO - implement these main states and set them up properly
    state->game_sm.add_state<Tetris::States::RunGameState>(Tetris::States::STATE_TETRIS, &state->game_sm, state->game.get(), state->renderer, &state->game_input,&state->game_input)
            .add_state<Tetris::States::MenuState>(Tetris::States::STATE_MAIN_MENU, &state->game_sm, state->renderer, &state->game_input,&state->game_input)
            .switch_to(Tetris::States::STATE_MAIN_MENU);
    // state->game_sm.add_state("rungame", std::move(state_run));
    // TetrisUtils::color_from_hex("#00E6FE");
    *appstate = state;

    std::cout << std::filesystem::current_path() << std::endl;
    // auto& jf = TEngine::Text::BitmapFonts::JoustFont::instance();
    // joust_tmp = jf.draw_char('H');
    // joust_tmp = jf.draw_string("HOLD");
    // joust_tmp = TEngine::Text::BitmapRenderer::draw_char<TEngine::Text::BitmapFonts::JoustFont>(state->renderer, 'H');
    // joust_tmp = TEngine::Text::BitmapRenderer::draw_string<TEngine::Text::BitmapFonts::JoustFont>(state->renderer, "HOLD");
    joust_tmp = state->bm_renderer->render_string<TEngine::Text::BitmapFonts::JoustFont>("HOLD");
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
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        state->game_input.read_input_event(event->key);
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

// static inline void do_input_working(AppState *state) {
//     if (state->game_input.has_input_event) {
//         if (state->game_input.key.hard_drop.pressed) {
//             state->game->do_hard_drop();
//         } else if (state->game_input.key.hold_piece.pressed) {
//             state->game->do_hold_piece();
//         } else {
//             auto [move, rot] = state->game_input.read_input_event(); // get movement and rotation from the keys that were just pressed
//             state->game->move(move);
//             state->game->rotate(rot);
//             // needed to prevent bizarrely fast movements, since movements are discrete rather than continuous
//         }
//         state->game_input.reset_events();
//     }
// }

// static inline void do_input_events(AppState *state) {
//     if (state->game_input.has_input_event) {
//         if (state->game_input.key.hard_drop.pressed) {
//             state->game->do_hard_drop();
//         } else if (state->game_input.key.hold_piece.pressed) {
//             state->game->do_hold_piece();
//         } else if (state->game_input.key.pause.pressed) {
//             // state->game->do_pause();
//         }
//         state->game_input.reset_events();
//     }
// }

// static inline void do_input_state(AppState *state) {
//     auto [move, rot] = state->game_input.read_input_state(); // get movement and rotation from the keys that were just pressed
//     state->game->move(move);
//     state->game->rotate(rot);
//     // sizeof(Tetris::Engine::GameInput::KeyState);
// }

// useful for continuous actions, like moving on a plane/space
/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = static_cast<AppState*>(appstate);
    double delta = state->time.delta_time();
    // do_input_working(state);
    // do_input_events(state);
    // do_input_state(state);
    // state->game->update(delta);
    // std::cout << 1.0/delta << std::endl;

    
    state->game_sm.handle_input();
    // std::cout << "trying\n";
    // state->game_sm.handle_input_state(state->game_input);
    state->game_sm.update(delta);

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    // SDL_RenderClear(state->renderer);
    // state->game->draw(state->renderer);
    state->game_sm.draw();
    // SDL_RenderFillRect(state->renderer, &rec);
    // SDL_RenderTextureRotated(state->renderer, state->playfield_texture, nullptr, nullptr, 0, nullptr, SDL_FLIP_VERTICAL);
    /* put the newly-cleared rendering on the screen. */
    SDL_FRect corner = {10,10,static_cast<float>(joust_tmp.text_data->w) * 2,static_cast<float>(joust_tmp.text_data->h) * 2};
    SDL_RenderTexture(state->renderer, joust_tmp.text_data, nullptr, &corner);
    SDL_RenderPresent(state->renderer);
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
    AppState *state = static_cast<AppState*>(appstate);
    // SDL_SetRenderVSync(state->renderer, true);
    // SDL_DestroyTexture(state->playfield_texture);
    // SDL_DestroyTexture(state->bag_texture);
    TEngine::Text::destroy_bitmap_text(joust_tmp);
    TEngine::Text::destroy_bitmap_renderer(state->bm_renderer);
    delete state; // state->game will be destroyed here
}