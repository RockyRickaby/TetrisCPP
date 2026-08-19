#include <SDL3/SDL_render.h>
#include <iostream>

#include "tetris_menu.hpp"
#include "../engine/text/text_renderer.hpp"

namespace Tetris {
    MainMenu::MainMenu(TEngine::Text::BitmapFont* font, SDL_Renderer* renderer) :
        m_renderer{renderer},
        m_font{font},
        m_wants_switch{false}
    {}
    
    void MainMenu::update(double delta_t) {
        m_offset_x += 30 * delta_t;
    }

    void MainMenu::event(TEngine::Events::IEvent& event) {
        using namespace TEngine::Events;
        EventDispatcher ev{event};
        ev.dispatch<KeyPressedEvent>([this](KeyPressedEvent& f_event){ return OnKeyPressed(f_event); });
        ev.dispatch<KeyRepeatEvent>([this](KeyRepeatEvent& f_event){ return OnKeyRepeat(f_event); });
    }

    void MainMenu::draw(void) {
        // TODO - implement drawing logic
        SDL_SetRenderDrawColor(m_renderer, 100, 0, 10, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(m_renderer);
        using namespace TEngine::Text;
        BitmapFontRenderer::draw_string_line(m_font, "MAIN MENU", 20 + m_offset_x, 20, 3);

    }

    bool MainMenu::wants_switch_state(void) {
        return m_wants_switch;
    }
    
    void MainMenu::reset(void) {
        m_offset_x = 0;
        m_wants_switch = false;
    }

    bool MainMenu::OnKeyPressed(TEngine::Events::KeyPressedEvent& event) {
        std::cout << "REGISTERED KEYPRESS EVENT\n";
        m_wants_switch = true;
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
    bool MainMenu::OnMouseDown(TEngine::Events::IEvent& event) {
        return false;
    }
}