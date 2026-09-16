#include <SDL3/SDL_render.h>
#include <iostream>

#include "tetris_menu.hpp"
#include "../engine/text/text_renderer.hpp"

// this tetris will always have the same logical resolution (I hope), so we may as well hardcode a few values...
// though it would be cool to make it work with different resolutions as well
namespace Tetris {
    MainMenu::MainMenu(
            TEngine::Text::BitmapFont* font,
            Tetris::Keybinds* keybinds,
            SDL_Renderer* renderer,
            std::span<Tetris::Piece3D> blocks_wireframe, 
            std::span<Tetris::Piece3D> blocks_filled
    ) :
        m_renderer{renderer},
        m_font{font},
        m_keybinds{keybinds},
        m_blocks_wireframe{blocks_wireframe},
        m_blocks_filled{blocks_filled},
        m_wants_switch{false},
        m_offset_y{720},
        m_blink{0.75f},
        m_read_any{false},
        m_blocks_framerate(1.0f/20.0f, true),
        m_switch_block{1.5f, true},
        m_block{m_blocks_wireframe.begin()}
    {}
    
    bool MainMenu::update(double delta_t) {
        if (m_offset_y >= 720.0f / 2.0f - (10 * m_font->tile_size())) {
            m_offset_y -= 80 * delta_t;
            for (auto& piece : m_blocks_wireframe) {
                auto& ins = piece.instance;
                auto off = piece.default_pos;
                ins.position = {ins.position.x , ((1 - (2*(m_offset_y + 80)) / 720.0f) * ins.position.z) + off.y, ins.position.z};
            }
            // m_blink = m_switch_block.get_time_left() - 0.75f;
        } else {
            // not really a press, but prevents having to double-tap space to actually start the game
            m_read_any = true;
            // m_blink -= delta_t;
            m_blink = m_switch_block.get_time_left() - 0.75f;
            if (m_blink <= -0.75) {
                m_blink = 0.75;
            }
        }
        if (m_blocks_framerate.done(delta_t)) {
            for (auto& piece : m_blocks_wireframe) {
                auto& ins = piece.instance;
                // ins.position = {0, 1 - m_offset_y / 100.0f, ins.position.z};
                auto off = piece.default_pos;
                ins.rotate_y(SDL_PI_F/4.0f * m_blocks_framerate.get_countdown_time());
                // ins.rotation.y = (SDL_PI_F/4.0f * 1);
            }
        }
        if (m_switch_block.done(delta_t)) {
            ++m_block;
            if (m_block == m_blocks_wireframe.end()) {
                m_block = m_blocks_wireframe.begin();
            }
        }
        return !m_wants_switch;
    }

    void MainMenu::event(TEngine::Events::IEvent& event) {
        using namespace TEngine::Events;
        
        // alternative version to instantiating an EventDispatcher
        dispatch_event<KeyRepeatEvent>(event, [this](KeyRepeatEvent& f_event){ return OnKeyRepeat(f_event); });
        dispatch_event<KeyPressedEvent>(event, [this](KeyPressedEvent& f_event){ return OnKeyPressed(f_event); });
    }

    void MainMenu::draw(void) {
        // TODO - implement drawing logic
        // SDL_SetRenderDrawColor(m_renderer, 100, 0, 10, SDL_ALPHA_OPAQUE);
        // SDL_RenderClear(m_renderer);
        using namespace TEngine::Text;
        // for (auto& ins : m_blocks_filled) {
        //     ins.draw_geometry({0,2,0}, 1.0f);
        // }
        // for (auto& ins : m_blocks_wireframe) {
        //     ins.draw_wireframe();
        // }
        m_block->draw({}, 1);

        const char str[] = "tetris";
        size_t twid = (sizeof(str) - 1);
        float scale = 10;
        BitmapFontRenderer::draw_string_line(m_font, str, (960 - scale * m_font->tile_size() * twid) / 2.0f, m_offset_y, scale);

        if (m_offset_y < 720.0f / 2.0f - (10 * m_font->tile_size()) && m_blink >= 0) {
            const char str2[] = "press spacebar to start";
            twid = (sizeof(str2) - 1);
            scale = 3;
            BitmapFontRenderer::draw_string_line(m_font, str2, (960 - scale * m_font->tile_size() * twid) / 2.0f, m_offset_y + 64 * scale, scale);
        }
    }

    void MainMenu::reset(void) {
        m_offset_y = 720;
        m_wants_switch = false;
        m_read_any = false;
        m_blocks_framerate.reset();
        m_switch_block.reset();
        m_block = m_blocks_wireframe.begin();
    }

    bool MainMenu::OnKeyPressed(TEngine::Events::KeyPressedEvent& event) {
        if (!m_read_any) {
            m_offset_y = 720.0f / 2.0f - (10 * m_font->tile_size());
            m_read_any = true;
        } else if (m_keybinds->keys.hold_piece.scancode == event.scancode) {
            if (m_offset_y < 720.0f / 2.0f - (10 * m_font->tile_size())) {
                m_wants_switch = true;
            }
        }
        return true;
    }

    bool MainMenu::OnKeyReleased(TEngine::Events::KeyReleasedEvent&) {
        return false;
    }

    bool MainMenu::OnKeyRepeat(TEngine::Events::KeyRepeatEvent& event) {
        std::cout << "REGISTERING KEYDOWN EVENT: " << event.scancode << std::endl;
        return true;
    }
    
    bool MainMenu::OnMousePressed(TEngine::Events::IEvent&) {
        return false;
    }

    bool MainMenu::OnMouseReleased(TEngine::Events::IEvent&) {
        return false;
    }

    // bool MainMenu::OnMouseDown(TEngine::Events::IEvent&) {
    //     return false;
    // }
}