#pragma once

#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <memory>
#include <span>
#include <unordered_map>
#include <array>
#include <SDL3/SDL.h>

#include "tetris_bags.hpp"
#include "tetris_pieces.hpp"
#include "../engine/tengine.hpp"
#include "../engine/sound/audio.hpp"
#include "../engine/text/fonts.hpp"
#include "../engine/text/text_renderer.hpp"

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
        concept PlayfieldCallbackRequirements = requires(PlayfieldRoutineCallback cb, int rows_cleared, int highest_line) {
            { cb(rows_cleared, highest_line) } -> std::same_as<void>;
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
        Game(
            SDL_Renderer* renderer,
            int screen_width,
            int screen_height,
            float block_scale,
            TEngine::Text::BitmapFont* font,
            SDL_Texture* mino_texturem,
            TEngine::Sound::Mixer* mixer
        );

        template <Bag::TetriminoQueue Q>
        void set_piece_queue() { m_pieces_bag = std::make_unique<Q>(m_renderer, m_mino_texture, m_queue_offset_x, m_queue_offset_y, m_scale / 1.5f); }

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

        struct ScoreSystemAdvanced {
            enum class ScoreActions : unsigned int {
                None = 0,
                Single = 1u << 0,
                Double = 1u << 1,
                Triple = 1u << 2,
                Tetris = 1u << 3,
                MiniTSpin = 1u << 4,
                TSpin = 1u << 5,
                SoftDrop = 1u << 6,
                HardDrop = 1u << 7,

                MiniTSpinSingle = MiniTSpin | Single,

                TSpinSingle = TSpin | Single,
                TSpinDouble = TSpin | Double,
                TSpinTriple = TSpin | Triple,
            };

            double speed = 1;
            double __default_speed = 1;
            int64_t score = 0;
            int level = 1;
            const int max_level = 15;
            const int __lines_to_clear_per_level_multiplier = 10;
            int __lines_cleared = 0;
            int __goal = 10;

            int __soft_lines = 0;
            int __hard_lines = 0;
            unsigned int __performed_actions;
            bool __is_back_to_back = false;

            bool is_5rot_spin = false;

            void erase_spin(void);
            void push_action(ScoreActions action);


            void update(void);
            void reset(void);
            void set_level(int new_level);
            double calculate_speed(void);
        };

        struct ScoreEventsTextManager {
            struct EventText {
                TEngine::Countdown visible_duration{1, false};
                TEngine::Vec2 position{};
                int text_idx = -1;
                float speed = 20;

                int time_to_speed();
            };
            std::array<TEngine::Text::BitmapText, 6> events_text{};
            std::vector<EventText> ongoing_texts{};
            Game* m_game_ptr;
            bool init = false;

            void reset(Game* game_ptr);
            void enable_texts_for(ScoreSystemAdvanced::ScoreActions action, int height = -1);
            void update(double delta_t);
            void draw();
        };

        struct SoundSystem {
            enum class AudioType {
                RegularLineClear,
                TetrisLineClear,
                Rotation
            };
            // void play_sound(int audio_idx);
        };

        struct PieceLockdownTimer {
            TEngine::Countdown timer = TEngine::Countdown{0.5, true};
            int moves = 15;
            float prev_y = -1;
            float piece_alpha = 1.0f;

            void reset(void);
        };

        struct PlayfieldUpdateRoutineAnimated {
            TEngine::Countdown fps = {0, true}; // 0.375
            // TEngine::Countdown fps; // 0.375

            template<typename Callback> requires(__Internal::PlayfieldCallbackRequirements<Callback>)
            bool operator()(Game *game_ptr, double delta_t, Callback scoreboard_callback) {
                // Game *game_ptr = static_cast<Game*>(game_ptr_v);
                if (m_act == Action::Begin) {
                    reset();
                    get_ranges(game_ptr);
                    for (const auto [x, y] : m_ranges_to_remove) {
                        m_cleared += y - x;
                    }
                    game_ptr->m_scoreboard_events.enable_texts_for(static_cast<ScoreSystemAdvanced::ScoreActions>(1u << (m_cleared - 1)), m_ranges_to_remove.back().second);
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
                    scoreboard_callback(m_cleared, m_ranges_to_remove.back().second);
                    pull_down(game_ptr);
                    return true;
                }
                return true;
            }
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

        struct PlayfieldUpdateRoutineGameOver {
            TEngine::Countdown animation_timer{0.08, true};
            TEngine::Vec2 black_screen_dimensions = {0, 0};
            TEngine::Vec2 black_screen_pos = {-100000, -100000};

            TEngine::OneShotTimer initial_delay{1.40};
            int row = 0;
            bool move_black_screen = false;

            bool operator()(Game* game_ptr, double delta_t);
            void draw(Game* game_ptr);
        };

        SDL_Renderer* m_renderer;
        inline static constexpr TEngine::Vec2 __invalid_move_vec = { 0, 0 };
        
        // these 3 floats will be changed in the constructor
        float m_scale = 20;
        float m_board_offset_x = ((32 - COLUMNS) / 2.0f) * m_scale;
        float m_board_offset_y = 1 * m_scale;

        float m_queue_offset_x = m_board_offset_x + (COLUMNS + 1.07f) * m_scale;
        float m_queue_offset_y = m_board_offset_y + 6 * m_scale;

        std::array<TEngine::Color, (ROWS + BUFFER) * COLUMNS> m_playfield_matrix = {};
        std::array<std::uint8_t, ROWS + BUFFER> m_line_block_count = {};
        
        // for rendering the playfield as a single geometry full of polygons (idk if this is good...)
        // improves performance when updated only when the playfield changes (not like it matters much in this case tho)
        std::vector<SDL_Vertex> m_verts;
        std::vector<int> m_indices;

        // Consider injecting this one from elsewhere (in the constructor)
        // ScoreSystem m_scoreboard;
        TEngine::Sound::Mixer* m_mixer;
        ScoreSystemAdvanced m_adv_scoreboard;
        ScoreEventsTextManager m_scoreboard_events;
        PlayfieldUpdateRoutineAnimated m_update_playfield_routine;
        PlayfieldUpdateRoutineInstant m_update_playfield_instant;
        PlayfieldUpdateRoutineGameOver m_update_playfield_gameover;
        // extended placement lock down variable
        // can also be used for infinite placement lock down or classic lock down if moves limit is ignored
        PieceLockdownTimer m_piece_lock;
        TEngine::Countdown m_piece_drop_timer = TEngine::Countdown{1, true};
        TEngine::Countdown m_piece_spawn = TEngine::Countdown{0, true};
        TEngine::Text::BitmapFont* m_text_font;
        // two pointer indirections happening here.
        SDL_Texture* m_mino_texture;

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

        bool try_move(bool &moved_down);
        bool try_rotate(void);        
        bool update_currentpiece(double delta_t, bool& placed);
        // move ghost piece down until we hit something
        void update_ghostpiece(void);
        bool update_playfield(double delta_t);
        bool spawn_next_piece(void);
        void place_piece(const Tetrimino::Piece& p);
        bool place_current_piece(void);
        bool place_and_spawn_next_piece(void);
        bool check_within_bounds(const Tetrimino::Piece &p);
        bool check_collision(const Tetrimino::Piece &p);
        // returns false when a respawn is not needed (or simply when the piece wasn't held)
        bool perform_hold();
        int perform_hard_drop(Tetrimino::Piece &p);
        bool perform_wallkick(Tetrimino::Piece &p);
        void check_and_award_tspin(const Tetrimino::Piece &p, int rot_point);
        void draw_piece(const Tetrimino::Piece& p, bool fill = true, Uint8 alpha = SDL_ALPHA_OPAQUE);
        void draw_held_piece(const Tetrimino::Piece& p, float offset_x, float offset_y, float scale);
        void draw_playfield_blocks(void);
        void draw_playfield_grid(void);
        void draw_text_elements(void);
        void regen_playfield(void);
    };
}