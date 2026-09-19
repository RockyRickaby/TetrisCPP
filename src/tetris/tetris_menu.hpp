#pragma once

#include <span>

#include "../engine/events.hpp"
#include "../engine/text/fonts.hpp"
#include "tetris_input.hpp"
#include "tetris_piece3d.hpp"

namespace Tetris {
    class MainMenu : public TEngine::Events::EventListener {
    public:
        MainMenu(
            TEngine::Text::BitmapFont* font,
            Tetris::Keybinds* keybinds,
            SDL_Renderer* renderer,
            std::span<Tetris::Piece3D> m_blocks_wireframe, 
            std::span<Tetris::Piece3D> m_blocks_filled
        );
        bool update(double delta_t);
        void event(TEngine::Events::IEvent& event) override;
        void draw(void);
        void reset(void);
    private:
        bool OnKeyPressed(TEngine::Events::KeyPressedEvent& event);
        bool OnKeyReleased(TEngine::Events::KeyReleasedEvent& event);
        bool OnKeyRepeat(TEngine::Events::KeyRepeatEvent& event);

        bool OnMousePressed(TEngine::Events::IEvent& event);
        bool OnMouseReleased(TEngine::Events::IEvent& event);
        // bool OnMouseDown(TEngine::Events::IEvent& event);

        SDL_Renderer* m_renderer;
        TEngine::Text::BitmapFont* m_font;
        Tetris::Keybinds* m_keybinds;
        std::span<Tetris::Piece3D> m_blocks_wireframe;
        std::span<Tetris::Piece3D> m_blocks_filled;
        TEngine::Countdown m_blocks_framerate;
        TEngine::Countdown m_switch_block;

        std::span<Tetris::Piece3D>::iterator m_block;

        bool m_wants_switch;
        float m_offset_y = 0;
        double m_blink = 0;

        bool m_read_any;
    };
}