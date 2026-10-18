#include "engine/tengine.hpp"
#include <print>
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h> // needed
#include <SDL3/SDL_main.h> // needed

#include "appstate.hpp"

// TODO - convert some pointer class members throughout the code into reference members!!!!
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
    SDL_AppResult appresult = SDL_APP_CONTINUE;
    AppState *state = static_cast<AppState*>(appstate);
    if (event->type == SDL_EVENT_QUIT) {
        appresult =  SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_Q) {
            appresult =  SDL_APP_SUCCESS;
        }
    }
    if (appresult == SDL_APP_SUCCESS) {
        SDL_Event quit_event = { .type = SDL_EVENT_QUIT };
        quit_event.quit.timestamp = SDL_GetTicksNS();
        
        state->raise_event(&quit_event);
    } else {
        state->raise_event(event);
    }
    return appresult;  /* carry on with the program! */
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
    AppState *state = static_cast<AppState*>(appstate);
    delete state; // state->game will be destroyed here
}