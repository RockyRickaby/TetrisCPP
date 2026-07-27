/*
 * DRAW FUNCTIONS WILL DRAW TO TEXTURES.
 * THESE SHOULD THEN BE DRAWN OVER THE SCREEN IN THE SDL_APPITERATE FUNCTION
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <memory>

// TODO - better separate headers and source files
#include "tetris.hpp"
#include "tetris_utils.hpp"
#include "tetris_engine.hpp"

using TetrisBag = Tetris::Bag::Standard;
struct AppState {
    /* We will use this renderer to draw into this window every frame. */
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *playfield_texture = nullptr;
    SDL_Texture *bag_texture = nullptr;
    std::unique_ptr<Tetris::Game<TetrisBag>> game = nullptr; // use new or make it static. size is a bit big
    
    Tetris::Engine::GameInput game_input;
    Tetris::Engine::Time time;
};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
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
    SDL_SetRenderLogicalPresentation(state->renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    // state->playfield_texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640 * 2, 480 * 2);
    // state->bag_texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 400, 200);
    
    state->game = std::make_unique<Tetris::Game<TetrisBag>>();
    // state->game->init(state->playfield_texture);
    state->game->init();
    // state->time = {};
    state->game_input.setup_keys(Tetris::Engine::GameInput::Keybinds::KEYBIND_DEFAULT);
    // TetrisUtils::color_from_hex("#00E6FE");
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
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        state->game_input.read_input(event->key);
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

// useful for continuous actions, like moving on a plane/space
/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = static_cast<AppState*>(appstate);
    double delta = state->time.delta_time();
    if (state->game_input.has_input) {
        if (state->game_input.key.hard_drop.pressed) {
            state->game->do_hard_drop();
        } else {
            auto [move, rot] = state->game_input.handle_input(); // get movement and rotation from the keys that were just pressed
            state->game->move(move);
            state->game->rotate(rot);
            // needed to prevent bizarrely fast movements, since movements are discrete rather than continuous
        }
        state->game_input.reset_states();
    }
    state->game->update(delta);

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    // SDL_RenderClear(state->renderer);
    state->game->draw(state->renderer);
    // SDL_RenderFillRect(state->renderer, &rec);
    SDL_RenderTextureRotated(state->renderer, state->playfield_texture, nullptr, nullptr, 0, nullptr, SDL_FLIP_VERTICAL);
    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(state->renderer);
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
    AppState *state = static_cast<AppState*>(appstate);
    SDL_DestroyTexture(state->playfield_texture);
    SDL_DestroyTexture(state->bag_texture);
    delete state; // state->game will be destroyed here
}