#pragma once

#include "../engine/tengine.hpp"
#include "../engine/events.hpp"
#include "../engine/text/fonts.hpp"

namespace Tetris {
    class MainMenu : public TEngine::Events::EventListener {
    public:
        MainMenu(TEngine::Text::BitmapFont* m_font, SDL_Renderer* renderer);
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
        TEngine::InputHandler m_stateinput_h;
        bool m_wants_switch;

        float m_offset_x = 0;
        double m_blink = 0;
    };
}