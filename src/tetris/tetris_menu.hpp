#pragma once

#include "../engine/tengine.hpp"
#include "../engine/events.hpp"
#include "../engine/text/fonts.hpp"

namespace Tetris {
    class MainMenu : public TEngine::Events::EventListener {
    public:
        MainMenu(TEngine::Text::BitmapFonts::Font* m_font, SDL_Renderer* renderer);
        void update(double delta_t);
        void event(TEngine::Events::IEvent& event) override;
        void draw(void);
        bool wants_switch_state(void);
        void reset(void);
    private:
        bool OnKeyPressed(TEngine::Events::KeyPressedEvent& event) override;
        bool OnKeyReleased(TEngine::Events::KeyReleasedEvent& event) override;
        bool OnKeyRepeat(TEngine::Events::KeyRepeatEvent& event) override;

        bool OnMousePressed(TEngine::Events::IEvent& event) override;
        bool OnMouseReleased(TEngine::Events::IEvent& event) override;
        bool OnMouseDown(TEngine::Events::IEvent& event) override;

        SDL_Renderer* m_renderer;
        TEngine::Text::BitmapFonts::Font* m_font;
        TEngine::InputHandler m_stateinput_h;
        bool m_wants_switch;
    };
}