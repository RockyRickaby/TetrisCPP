#pragma once

#include <memory>
#include <filesystem>
#include <SDL3/SDL.h>

#include "engine/tengine.hpp"
#include "tetris/tetris_game.hpp"
#include "tetris/tetris_menu.hpp"
#include "tetris/tetris_input.hpp"
#include "tetris/tetris_state_machines/tetris_game_sm.hpp"

struct Experimental;

class AppState {
public:
    void raise_event(SDL_Event* event);
    SDL_AppResult update_and_draw();
    SDL_AppResult setup_app();

    ~AppState();

private:
    /* We will use this renderer to draw into this window every frame. */
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    // TEngine::Text::BitmapRenderer bm_renderer;
    // SDL_Texture *playfield_texture = nullptr;
    // SDL_Texture *bag_texture = nullptr;
    std::unique_ptr<Tetris::Game> game = nullptr; // use new or make it static. size is a bit big
    std::unique_ptr<Tetris::MainMenu> menu = nullptr; // use new or make it static. size is a bit big
    std::unique_ptr<Tetris::States::TetrisStateMachine> game_sm;
    
    TEngine::Text::BitmapFont font;
    TEngine::Sprites::SpriteAtlas spratlas;
    // std::unique_ptr<TEngine::Sprites::SpriteEntity> entity;
    Tetris::Keybinds tetris_keys;

    TEngine::Time time;
    TEngine::TextureWrapper mino_texture;

    Experimental* __testing = nullptr;

    std::filesystem::path assets_root = std::filesystem::path("assets");
    std::filesystem::path fonts_root = assets_root / "fonts"; 
    std::filesystem::path models_root = assets_root / "sillymodels";

    int window_width = 800;
    int window_height = 600;
};