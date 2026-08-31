#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "appstate.hpp"

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    AppState* ps = new AppState();
    *appstate = ps;
    return ps->setup_app();  /* carry on with the program! */
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
    state->raise_event(event);
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

// useful for continuous actions, like moving on a plane/space
/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = static_cast<AppState*>(appstate);
    return state->update_and_draw();
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    using namespace TEngine;
    /* SDL will clean up the window/renderer for us. */
    AppState *state = static_cast<AppState*>(appstate);
    delete state; // state->game will be destroyed here
}