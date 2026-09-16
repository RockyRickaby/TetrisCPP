#pragma once

#include <SDL3/SDL.h>
#include <span>

#include "../../engine/state_machine.hpp"
// #include "../../engine/tengine.hpp"
#include "../../engine/events.hpp"
#include "../../engine/text/fonts.hpp"
#include "../tetris_scoreboard.hpp"
#include "../tetris_game.hpp"
#include "../tetris_input.hpp"
#include "../tetris_menu.hpp"

namespace Tetris::States {
    inline static const int STATE_MAIN_MENU = 0;
    inline static const int STATE_TETRIS = 1;
    inline static const int STATE_GAMEOVER = 2;

    class TetrisStateMachine;

    class MenuState final : public TEngine::StateMachine::State {
    public:
        MenuState(
            TetrisStateMachine *sm,
            Game* game,
            MainMenu* menu,
            Tetris::Keybinds* tetris_keys,
            SDL_Renderer *renderer,
            TEngine::Text::BitmapFont* font
        ) :
            m_game_ptr{game},
            m_sm_ptr{sm},
            m_menu{menu},
            m_tetris_keys(tetris_keys),
            m_font{font},
            m_renderer{renderer},
            tmp_counter{10},
            m_read_any{true}
        {}

        void enter(void) override;
        void update(double delta_t) override;
        // void handle_input(void) override;
        void event(TEngine::Events::IEvent& event) override;
        void draw(void) override;
        void reset(void) override;
        void exit(void) override;
    private:
        bool OnKeyPressed(TEngine::Events::KeyPressedEvent& event);

        Game *m_game_ptr;
        TetrisStateMachine *m_sm_ptr;
        MainMenu* m_menu;
        Tetris::Keybinds *m_tetris_keys;
        TEngine::Text::BitmapFont* m_font;
        SDL_Renderer *m_renderer;
        TEngine::OneShotTimer tmp_counter;
        bool m_read_any;
    };









    class RunGameState final : public TEngine::StateMachine::State {
    public:
        RunGameState(TetrisStateMachine* sm, Game* game, Tetris::Keybinds* tetris_keys, TEngine::Text::BitmapFont* font, SDL_Renderer* renderer) :
            m_run_state(),
            m_game_ptr{game},
            m_parent_sm(sm),
            m_tetris_keys(tetris_keys),
            m_renderer{renderer},
            m_font{font},
            m_begin_countdown{3},
            m_pause_countdown{0},
            m_go_count{1},
            m_pause{false}
        {}

        void enter() override;
        void update(double delta_t) override;
        // void handle_input() override;
        void event(TEngine::Events::IEvent& event) override;
        void draw() override;
        void reset(void) override;
        void exit() override;

    private:
        // not needed for now
        enum class State {
            Begin,
            Running,
            Pause
        };
        State m_run_state;

        bool OnKeyPressed(TEngine::Events::KeyPressedEvent& event);

        Game *m_game_ptr;
        TetrisStateMachine *m_parent_sm;
        Tetris::Keybinds* m_tetris_keys;
        SDL_Renderer *m_renderer;
        TEngine::Text::BitmapFont* m_font;
        
        TEngine::Countdown m_begin_countdown;
        TEngine::Countdown m_pause_countdown;
        
        TEngine::OneShotTimer m_go_count;

        bool m_pause;
    };









    class GameOverState final : public TEngine::StateMachine::State {
    public:
        GameOverState(TetrisStateMachine *sm, Game* game, SDL_Renderer *renderer, TEngine::Text::BitmapFont* font, Score::Leaderboard& leaderboard, std::span<Tetris::Piece3D> blocks_wireframe) :
            m_has_highscore(false),
            m_game_ptr{game},
            m_sm_ptr{sm},
            // m_input_event_handler(input_event_h),
            m_font{font},
            m_blocks_wireframe{blocks_wireframe},
            m_block{m_blocks_wireframe.begin()},
            m_renderer{renderer},
            m_gameover_text_pos{},
            m_newscore_text_pos{},
            m_leaderboard_offset{},
            m_gameover_timer(0),
            m_highscore_delay{0},
            m_leaderboard_transition_delay{0},
            m_leaderboard(leaderboard),
            m_blocks_framerate(1.0f/20.0f, true),
            m_switch_block{1.5f, true}
        {}

        void enter(void) override;
        void update(double delta_t) override;
        void event(TEngine::Events::IEvent& event) override;
        // void handle_input(void) override;
        void draw(void) override;
        void reset(void) override;
        void exit(void) override;
    private:
        bool m_has_highscore;
        Game *m_game_ptr;
        TetrisStateMachine *m_sm_ptr;
        TEngine::Text::BitmapFont* m_font;
        std::span<Tetris::Piece3D> m_blocks_wireframe;
        std::span<Tetris::Piece3D>::reverse_iterator m_block;
        SDL_Renderer *m_renderer;
        TEngine::Vec2 m_gameover_text_pos;
        TEngine::Vec2 m_newscore_text_pos;
        TEngine::Vec2 m_leaderboard_offset;
        TEngine::OneShotTimer m_gameover_timer;
        TEngine::OneShotTimer m_highscore_delay;
        TEngine::OneShotTimer m_leaderboard_transition_delay;
        Score::Leaderboard m_leaderboard;
        Score::InMemoryLeaderboard m_lb_storage;

        TEngine::Countdown m_blocks_framerate;
        TEngine::Countdown m_switch_block;
    };









    // dedicated state machine for handling everything needed for the game to run
    class TetrisStateMachine : public TEngine::StateMachine::StateMachine {
    public:
        TetrisStateMachine(
            Tetris::Game* game_ptr,
            Tetris::MainMenu* menu_ptr,
            SDL_Renderer* renderer,
            Tetris::Keybinds* tetris_keys,
            TEngine::Text::BitmapFont* font,
            Score::Leaderboard& leaderboard,
            std::span<Tetris::Piece3D> blocks_wireframe,
            std::span<Tetris::Piece3D> blocks_filled
        );
        void update(double delta_t) override;
        // void handle_input(void) override { m_current->handle_input(); }
        void event(TEngine::Events::IEvent& event) override { m_current->event(event); }
        void draw(void) override { m_current->draw(); }
        void switch_to(int id);
    private:
        void update_state(void);

        RunGameState m_runstate;
        MenuState m_menustate;
        GameOverState m_gameoverstate;

        std::array<TEngine::StateMachine::State*, 3> m_states;
        TEngine::StateMachine::State* m_current;
        // maybe we'll use these
        std::span<Tetris::Piece3D> m_blocks_wireframe;
        std::span<Tetris::Piece3D> m_blocks_filled;
        bool m_switching;
        int m_next;
    };
}