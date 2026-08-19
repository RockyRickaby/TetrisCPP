#pragma once

#include <memory>
#include <unordered_map>
#include <array>
#include <SDL3/SDL.h>

#include "tetris_bags.hpp"
#include "tetris_pieces.hpp"
#include "../engine/tengine.hpp"
#include "../engine/text/fonts.hpp"

namespace Tetris {
    namespace __Internal {
        // Implemented in kicks.cpp
        namespace WallkickData {
            // some of the kick tests are the exact same for some rotations
            // only contains non-zero tests (no test TEngine::Vec2 is equal to {0,0})
            extern const std::unordered_map<int, std::array<TEngine::Vec2, 4>> WALLKICKS_ANY;
            // some of the kick tests are the exact same for some rotations
            // only contains non-zero tests (no test is equal to {0,0})
            extern const std::unordered_map<int, std::array<TEngine::Vec2, 4>> WALLKICKS_I;
        }

        // Currently, we do not concern ourselves about T-spins and other actions that affect the score
        template<typename PlayfieldRoutineCallback>
        concept PlayfieldCallbackRequirements = requires(PlayfieldRoutineCallback cb, int rows_cleared) {
            { cb(rows_cleared) } -> std::same_as<void>;
        };
    };

    // doesn't listen to events... it doesn't have to
    // matrix grows UPWARDS and to the right. must be flipped when rendered
    // note: not trivially copyable (duh)
    class Game {
    public:
        static constexpr int ROWS = 22;
        static constexpr int COLUMNS = 10;
        static constexpr int BUFFER = 5;

        // constructor initializes the game. it will be ready to run
        Game(SDL_Renderer* renderer, int screen_width, int screen_height, float block_scale, TEngine::Text::BitmapFont* font);

        template <Bag::TetriminoQueue Q>
        void set_piece_queue() { m_pieces_bag = std::make_unique<Q>(); }

        void restart(void);
        bool update(double delta_t);
        void move(TEngine::Vec2 dir);
        void rotate(Tetrimino::Rotation r) { m_nextrot = r; }
        void do_hard_drop(void) { m_hard_drop = true; }
        void do_hold_piece(void) { m_hold_piece = true; }
        void set_level(int level);
        int64_t get_score(void);
        // drawing logic is vertically mirrored
        // the playfield grows upwards, but the drawing grows downwards, so we flip it
        void draw(void);
    private:
        enum class GameUpdateState {
            GPiece,
            GPlayfield,
            GSpawn,
            GGameover
        };
        // leave everything exposed
        // TODO - maybe use the custom event listener to handle stuff like t-spins
        struct ScoreSystem {
            int64_t score = 0;
            int level = 1;
            int max_level = 15;
            int lines_to_clear_per_level_multiplier = 10;
            int lines_cleared = 0;
            int goal = 10;
            double speed = 1;
            double default_speed = 1;

            void update(int cleared);
            void reset(void);
            void set_level(int level);
            double calculate_speed(void);
        };

        struct PieceLockdownTimer {
            TEngine::Countdown timer = TEngine::Countdown{0.5, true};
            int moves = 15;
            float prev_y = -1;

            void reset(void) {
                timer.reset();
                moves = 15;
                prev_y = -1;
            }
        };

        class PlayfieldUpdateRoutineAnimated {
        public:
            TEngine::Countdown fps = {0, true}; // 0.375
            // TEngine::Countdown fps; // 0.375

            template<typename Callback> requires(__Internal::PlayfieldCallbackRequirements<Callback>)
            bool operator()(Game *game_ptr, double delta_t, Callback scoreboard_callback) {
                // Game *game_ptr = static_cast<Game*>(game_ptr_v);
                if (m_act == Action::Begin) {
                    reset();
                    get_ranges(game_ptr);
                    fps.set_countdown_time(0.5/COLUMNS);
                    m_act = Action::Clearing;
                    return false;
                } else if (m_act == Action::Clearing) {
                    if (!fps.done(delta_t)) {
                        return false;
                    }
                    if (m_clearing_l < 0 || m_clearing_r >= COLUMNS) {
                        m_act = Action::Ending;
                        return false;
                    }
                    for (auto it = m_ranges_to_remove.begin(); it != m_ranges_to_remove.end(); ++it) {
                        auto [fst, snd] = *it;
                        for (int i = fst; i < snd; i++) {
                            game_ptr->m_playfield_matrix[i * COLUMNS + m_clearing_l] = TEngine::Color{};
                            game_ptr->m_playfield_matrix[i * COLUMNS + m_clearing_r] = TEngine::Color{};
                        }
                    }
                    m_clearing_l -= 1;
                    m_clearing_r += 1;
                    return false;
                } else if (m_act == Action::Ending) {
                    m_act = Action::Begin;
                    pull_down(game_ptr);
                    scoreboard_callback(m_cleared);
                    return true;
                }
                return true;
            }
        private:
            enum class Action {
                Begin,
                Clearing,
                Ending
            };
            
            std::vector<std::pair<int, int>> m_ranges_to_remove;
            Action m_act = Action::Begin;
            int m_clearing_r = 0;
            int m_clearing_l = 0;
            int m_cleared = 0;
            
            void reset(void);
            void get_ranges(Game *game_ptr);
            void pull_down(Game *game_ptr);
        };

        // updates playfield, but with no animations...
        // TODO - the pull_down logic is the same as in the other routine... consider way to not have it duplicated
        struct PlayfieldUpdateRoutineInstant {
            template<typename Callback> requires(__Internal::PlayfieldCallbackRequirements<Callback>)
            bool operator()(Game *game_ptr, double delta_t, Callback scoreboard_callback) {
                std::array<std::pair<int, int>, 4> ranges_to_remove;
                int count = get_ranges(game_ptr, ranges_to_remove);
                int cleared = pull_rows(game_ptr, ranges_to_remove, count);
                scoreboard_callback(cleared);
                return true;
            }
            int get_ranges(Game *game_ptr, std::array<std::pair<int, int>, 4>& ranges_to_remove);
            int pull_rows(Game* game_ptr, std::array<std::pair<int, int>, 4>& ranges_to_remove, int size);
        };
        SDL_Renderer* m_renderer;
        inline static constexpr TEngine::Vec2 __invalid_move_vec = { 0, 0 };
        
        // these 3 floats will be changed in the constructor
        float m_scale = 20;
        float m_board_offset_x = ((32 - COLUMNS) / 2.0f) * m_scale;
        float m_board_offset_y = 1 * m_scale;

        std::array<TEngine::Color, (ROWS + BUFFER) * COLUMNS> m_playfield_matrix = {};
        std::array<std::uint8_t, ROWS + BUFFER> m_line_block_count = {};
        
        // for rendering the playfield as a single geometry full of polygons (idk if this is good...)
        // improves performance when updated only when the playfield changes (not like it matters much in this case tho)
        std::vector<SDL_Vertex> m_verts;
        std::vector<int> m_indices;

        // Consider injecting this one from elsewhere (in the constructor)
        ScoreSystem m_scoreboard;
        PlayfieldUpdateRoutineAnimated m_update_playfield_routine;
        PlayfieldUpdateRoutineInstant m_update_playfield_instant;
        // extended placement lock down variable
        // can also be used for infinite placement lock down or classic lock down if moves limit is ignored
        PieceLockdownTimer m_piece_lock;
        TEngine::Countdown m_piece_drop_timer = TEngine::Countdown{1, true};
        TEngine::Countdown m_piece_spawn = TEngine::Countdown{0, true};
        TEngine::Text::BitmapFont* m_text_font;

        Tetrimino::Piece m_current{};
        Tetrimino::Piece m_ghost{};
        Tetrimino::Piece m_held{};
        std::unique_ptr<Bag::Queue> m_pieces_bag;

        Tetrimino::Rotation m_nextrot = Tetrimino::Rotation::None;
        TEngine::Vec2 m_nextdir = __invalid_move_vec;

        GameUpdateState m_update_state = {};
        bool m_hard_drop = false;
        bool m_hold_piece = false;
        bool m_hold_recently_swapped = false;

        void reset_piece_lock_timer() { m_piece_lock.reset(); }
        bool try_move(bool &moved_down);
        bool try_rotate(void);        
        bool update_currentpiece(double delta_t, bool& placed);
        // move ghost piece down until we hit something
        void update_ghostpiece(void);
        bool update_playfield(double delta_t);
        bool spawn_next_piece(void);
        bool place_current_piece(void);
        bool place_and_spawn_next_piece(void);
        bool check_within_bounds(const Tetrimino::Piece &p);
        bool check_collision(const Tetrimino::Piece &p);
        // returns false when a respawn is not needed (or simply when the piece wasn't held)
        bool perform_hold();
        void perform_hard_drop(Tetrimino::Piece &p);
        bool perform_wallkick(Tetrimino::Piece &p);
        void draw_piece(const Tetrimino::Piece& p, bool fill = true, Uint8 alpha = SDL_ALPHA_OPAQUE);
        void draw_held_piece(const Tetrimino::Piece& p, float offset_x, float offset_y, float scale);
        void draw_playfield_blocks(void);
        void draw_playfield_grid(void);
        void draw_text_elements(void);
        void regen_playfield(void);
    };
}