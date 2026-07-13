/*
 * This example code creates an SDL window and renderer, and then clears the
 * window to a different color every frame, so you'll effectively get a window
 * that's smoothly fading between colors.
 *
 * This code is public domain. Feel free to use it for any purpose!
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

#include "tetris.hpp"
#include "tetris_utils.hpp"

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
Tetris::Tetrimino::Piece* k;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Tetris", 640, 480, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    std::cout << TetrisUtils::from_hex("#FFDE00") << '\n';
    k = new Tetris::Tetrimino::Piece(Tetris::Tetrimino::DEFAULT_PIECES.at(Tetris::Tetrimino::Type::I));
    for (const Tetris::Block& b : k->get_blocks()) {
        std::cout << b.pos << ' ';
    }
    std::cout << "\n";
    // k->move({0, 0});
    // k->rotate(Tetris::Tetrimino::Rotation::COUNTERCLOCKWISE);
    // k->rotate(Tetris::Tetrimino::Rotation::COUNTERCLOCKWISE);
    for (const Tetris::Block& b : k->get_blocks()) {
        std::cout << b.pos << ' ';
    }
    std::cout << "\n";
    std::vector<Tetris::Block> bl{};
    size_t v = k->get_blocks(bl);
    for (const Tetris::Block& b : bl) {
        std::cout << b.pos << ' ';
    }
    std::cout << "\n";
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        auto *keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_ESCAPE]) {
            return SDL_APP_SUCCESS;
        }
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    const double now = ((double)SDL_GetTicks()) / 1000.0;  /* convert from milliseconds to seconds. */
    /* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
    const float red = (float) (0.5 + 0.5 * SDL_sin(now));
    const float green = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    const float blue = (float) (0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
    SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */

    /* clear the window to the draw color. */
    SDL_RenderClear(renderer);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    using namespace Tetris;
    std::vector<Block> bl{0};
    k->get_blocks(bl);
    for (Block &b : bl) {
        SDL_FRect r{ .x = b.pos.x * 20, .y = 40 -b.pos.y * 20, .w = 20, .h = 20};
        SDL_SetRenderDrawColor(renderer, b.color.r, b.color.g, b.color.b, SDL_ALPHA_OPAQUE);
        SDL_RenderRect(renderer, &r);
    }
    // SDL_RenderFillRect(renderer, &rec);
    
    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
    delete k;
}