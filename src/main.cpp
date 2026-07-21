/*
 * DRAW FUNCTIONS WILL DRAW TO TEXTURES.
 * THESE SHOULD THEN BE DRAWN OVER THE SCREEN IN THE SDL_APPITERATE FUNCTION
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <memory>

#include "tetris.hpp"
#include "tetris_base.hpp"
#include "tetris_utils.hpp"

/* We will use this renderer to draw into this window every frame. */
// static SDL_Window *window = NULL;
// static SDL_Renderer *renderer = NULL;

// Tetris::Game<Tetris::Bag::OnePiece<Tetris::Tetrimino::Type::I>> game;

// static Uint64 freq;
// static Uint64 time_last{0};

SDL_Texture *playfield = nullptr;

// constructor should be called after initializing SDL
struct Time {
    Uint64 freq;
    Uint64 time_last;
    double delta_time;

    Time() :
        freq{SDL_GetPerformanceFrequency()},
        time_last{SDL_GetPerformanceCounter()},
        delta_time{0}
    {}

    double update_delta(void) {
        Uint64 time_now = SDL_GetPerformanceCounter();
        delta_time = (static_cast<double>(time_now - time_last) / static_cast<double>(freq));
        time_last = time_now;
        return delta_time;
    }
};

struct Input {
    enum class Keybinds {
        KEYBIND_DEFAULT, // uses arrow keys
        KEYBIND_IJKL,
    };
    bool has_input = false;
    Tetris::Vec2 move_vec{};
    Tetris::Tetrimino::Rotation rotation{};

    // TODO - merge this with the next struct
    SDL_Scancode key_left = SDL_SCANCODE_UNKNOWN;
    SDL_Scancode key_right = SDL_SCANCODE_UNKNOWN;
    SDL_Scancode key_down = SDL_SCANCODE_UNKNOWN; // TODO soft drop. should be continuous (?) (auto-repeat?)
    SDL_Scancode key_autorepeat = SDL_SCANCODE_UNKNOWN; // might not be used. for allowing a continuous press to move the piece quickly. might be unnecessary
    SDL_Scancode key_hard_drop = SDL_SCANCODE_UNKNOWN;
    SDL_Scancode key_rotate_clockwise = SDL_SCANCODE_UNKNOWN;
    SDL_Scancode key_rotate_counterclockwise = SDL_SCANCODE_UNKNOWN;
    SDL_Scancode key_hold_piece = SDL_SCANCODE_UNKNOWN;
    SDL_Scancode key_pause = SDL_SCANCODE_UNKNOWN;

    struct Pressed {
        bool key_left_pressed = false;
        bool key_right_pressed = false;
        // TODO - soft drop. should be continuous (?) (auto-repeat?)
        bool key_down_pressed = false;
        // might not be used. for allowing a continuous press to move the piece quickly. might be unnecessary. should be prioritized over everything
        bool key_autorepeat_pressed = false;
        bool key_hard_drop_pressed = false;
        bool key_rotate_clockwise_pressed = false;
        bool key_rotate_counterclockwise_pressed = false;
        bool key_hold_piece_pressed = false;
        bool key_pause_pressed = false;
    } state{};

    bool setup_keys(Keybinds bind_settings);
    void read_input(const SDL_KeyboardEvent &keyboard);
    void handle_input(void);
    void reset(void);
};

struct AppState {
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *playfield_texture = nullptr;
    SDL_Texture *bag_texture = nullptr;
    std::unique_ptr<Tetris::Game<Tetris::Bag::OnePiece<Tetris::Tetrimino::Type::I>>> game = nullptr; // use new or make it static. size is a bit big

    Input input;
    Time time;
};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    AppState *state = new AppState();
    if (!SDL_CreateWindowAndRenderer("Tetris", 640, 480, SDL_WINDOW_RESIZABLE, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(state->renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    state->playfield_texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640 * 2, 480 * 2);
    state->bag_texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 400, 200);
    
    state->game = std::make_unique<Tetris::Game<Tetris::Bag::OnePiece<Tetris::Tetrimino::Type::I>>>();
    state->game->init(state->playfield_texture);
    
    // state->time = {};
    state->input.setup_keys(Input::Keybinds::KEYBIND_DEFAULT);
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

    if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
        state->input.read_input(event->key);
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

// useful for continuous actions, like moving on a plane/space
/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = static_cast<AppState*>(appstate);
    double delta = state->time.update_delta();
    if (state->input.has_input) {
        if (state->input.state.key_hard_drop_pressed) {
            state->game->do_hard_drop();
        } else {
            state->input.handle_input(); // get movement and rotation from the keys that were just pressed
            state->game->move(state->input.move_vec);
            state->game->rotate(state->input.rotation);
            // needed to prevent bizarrely fast movements, since movements are discrete rather than continuous
        }
        state->input.reset();
    }
    state->game->update(delta);

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(state->renderer);
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


bool Input::setup_keys(Keybinds bind_settings) {
    if (bind_settings == Keybinds::KEYBIND_DEFAULT) {
        key_left = SDL_SCANCODE_LEFT;
        key_right = SDL_SCANCODE_RIGHT;
        key_down = SDL_SCANCODE_DOWN;
        key_hard_drop = SDL_SCANCODE_UP;
    } else if (bind_settings == Keybinds::KEYBIND_IJKL) {
        key_left = SDL_SCANCODE_J;
        key_right = SDL_SCANCODE_L;
        key_down = SDL_SCANCODE_K;
        key_hard_drop = SDL_SCANCODE_I;
    }
    key_autorepeat = SDL_SCANCODE_UNKNOWN;
    key_rotate_counterclockwise = SDL_SCANCODE_D;
    key_rotate_clockwise = SDL_SCANCODE_F;
    key_hold_piece = SDL_SCANCODE_SPACE;
    key_pause = SDL_SCANCODE_P;
    return true;
}

void Input::read_input(const SDL_KeyboardEvent &keyboard) {
    // TODO - allow soft drop and auto-repeat for sideways movement
    if (keyboard.down) {
        if (!keyboard.repeat) {
            SDL_Scancode code = keyboard.scancode;
            // TODO - set other keys' states
            state.key_left_pressed = code == key_left;
            state.key_right_pressed = code == key_right;
            state.key_down_pressed = code == key_down;
            state.key_hard_drop_pressed = code == key_hard_drop;
            state.key_rotate_clockwise_pressed = code == key_rotate_clockwise;
            state.key_rotate_counterclockwise_pressed = code == key_rotate_counterclockwise;
        }
        has_input = true;
    } else {
        has_input = false;
    }
}

void Input::handle_input(void) {
    using Tetris::Vec2;
    using Tetris::Tetrimino::Rotation;
    if (!has_input) {
        reset();
    }
    // process one at a time. no diagonals allowed
    if (state.key_left_pressed) {
        move_vec = Vec2{-1,0};
    } else if (state.key_right_pressed) {
        move_vec = Vec2{1,0};
    } else if (state.key_down_pressed) {
        move_vec = Vec2{0,-1};
    } else if (state.key_rotate_clockwise_pressed) {
        rotation = Rotation::CLOCKWISE;
    } else if (state.key_rotate_counterclockwise_pressed) {
        rotation = Rotation::COUNTERCLOCKWISE;
    } else {
    }
}

void Input::reset(void) {
    move_vec = Tetris::Vec2{};
    rotation = Tetris::Tetrimino::Rotation::NONE;
    has_input = false;
    state = {};
}