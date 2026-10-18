#pragma once

#include <memory>
#include <filesystem>
#include <SDL3/SDL.h>

#include "engine/tengine.hpp"
#include "engine/silly3D/silly_3D.hpp"
#include "engine/sound/audio.hpp"
#include "tetris/tetris_game.hpp"
#include "tetris/tetris_menu.hpp"
#include "tetris/tetris_input.hpp"
#include "tetris/tetris_piece3d.hpp"
#include "tetris/tetris_scoreboard.hpp"
#include "tetris/tetris_state_machines/tetris_game_sm.hpp"

struct Experimental;

class AppState {
public:
    void raise_event(SDL_Event* event);
    SDL_AppResult update_and_draw();
    SDL_AppResult setup_app();

    void load_sounds();

    ~AppState();
private:
    /* We will use this renderer to draw into this window every frame. */
    TEngine::Window window;
    TEngine::AppMetadata metadata;

    std::vector<Tetris::Piece3D> pieces3dwire;
    std::vector<Tetris::Piece3D> pieces3dfill;

    Tetris::Keybinds tetris_keys;
    std::unique_ptr<Tetris::Game> game = nullptr; // use new or make it static. size is a bit big
    std::unique_ptr<Tetris::MainMenu> menu = nullptr; // use new or make it static. size is a bit big
    std::unique_ptr<Tetris::States::TetrisStateMachine> game_sm;
    
    TEngine::Time time;
    TEngine::Text::BitmapFont font;
    TEngine::TextureWrapper mino_texture;
    TEngine::Silly3D::SillyAssetManager silly_assets3d;

    std::filesystem::path assets_root = std::filesystem::path("assets");
    std::filesystem::path fonts_root = assets_root / "fonts"; 
    std::filesystem::path sounds_root = assets_root / "sounds"; 
    std::filesystem::path models_root = assets_root / "sillymodels";
    std::filesystem::path pieces_root = assets_root / "sillymodels" / "pieces";

    Tetris::Score::Leaderboard leaderboard;

    TEngine::Sound::Mixer mixer;

    std::vector<TEngine::Sound::Audio> sfx;
    std::vector<TEngine::Sound::Audio> music;
    std::vector<TEngine::Sound::Track> tracks_from_music;
    std::vector<TEngine::Sound::Track> tracks_sfx;
    
    Experimental* __testing = nullptr;
};