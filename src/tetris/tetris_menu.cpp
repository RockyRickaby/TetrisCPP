#include <SDL3/SDL_render.h>
#include <iostream>

#include "tetris_menu.hpp"
#include "../engine/text/text_renderer.hpp"

namespace Tetris {
    MainMenu::MainMenu(TEngine::Text::BitmapFont* font, SDL_Renderer* renderer) :
        m_renderer{renderer},
        m_font{font},
        m_stateinput_h(),
        m_wants_switch{false},
        m_offset_x{-(10.0f * m_font->tile_size())},
        m_blink{0.75f}
    {}
    
    bool MainMenu::update(double delta_t) {
        if (m_offset_x < 720.0f / 2.0f - (10 * m_font->tile_size())) {
            m_offset_x += 60 * delta_t;
        } else {
            m_blink -= delta_t;
            if (m_blink <= -0.75) {
                m_blink = 0.75;
            }
        }
        return !m_wants_switch;
    }

    void MainMenu::event(TEngine::Events::IEvent& event) {
        using namespace TEngine::Events;
        EventDispatcher ev{event};
        ev.dispatch<KeyPressedEvent>([this](KeyPressedEvent& f_event){ return OnKeyPressed(f_event); });
        ev.dispatch<KeyRepeatEvent>([this](KeyRepeatEvent& f_event){ return OnKeyRepeat(f_event); });
    }

    void MainMenu::draw(void) {
        // TODO - implement drawing logic
        // SDL_SetRenderDrawColor(m_renderer, 100, 0, 10, SDL_ALPHA_OPAQUE);
        // SDL_RenderClear(m_renderer);
        using namespace TEngine::Text;

        const char str[] = "tetris";
        size_t twid = (sizeof(str) - 1);
        float scale = 10;
        BitmapFontRenderer::draw_string_line(m_font, str, (960 - scale * m_font->tile_size() * twid) / 2.0f, m_offset_x, scale);

        if (m_offset_x >= 720.0f / 2.0f - (10 * m_font->tile_size()) && m_blink >= 0) {
            const char str2[] = "press spacebar to start";
            twid = (sizeof(str2) - 1);
            scale = 3;
            BitmapFontRenderer::draw_string_line(m_font, str2, (960 - scale * m_font->tile_size() * twid) / 2.0f, m_offset_x + 64 * scale, scale);
        }
    }

    void MainMenu::reset(void) {
        m_offset_x = -(10 * m_font->tile_size());
        m_wants_switch = false;
    }

    bool MainMenu::OnKeyPressed(TEngine::Events::KeyPressedEvent& event) {
        if (m_offset_x >= 720.0f / 2.0f - (10 * m_font->tile_size())) {
            m_wants_switch = true;
        }
        m_offset_x = 720.0f / 2.0f - (10 * m_font->tile_size());
        return true;
    }

    bool MainMenu::OnKeyReleased(TEngine::Events::KeyReleasedEvent& event) {
        return false;
    }

    bool MainMenu::OnKeyRepeat(TEngine::Events::KeyRepeatEvent& event) {
        std::cout << "REGISTERING KEYDOWN EVENT: " << event.get_scancde() << std::endl;
        return true;
    }
    
    bool MainMenu::OnMousePressed(TEngine::Events::IEvent& event) {
        return false;
    }

    bool MainMenu::OnMouseReleased(TEngine::Events::IEvent& event) {
        return false;
    }

    // bool MainMenu::OnMouseDown(TEngine::Events::IEvent& event) {
    //     return false;
    // }
}