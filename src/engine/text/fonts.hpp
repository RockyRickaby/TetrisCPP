#pragma once

#include <unordered_map>
#include <SDL3/SDL.h>

// TODO - fonts should really just hold enough stuff for drawing the characters... at least in this case
namespace TEngine::Text::BitmapFonts {
    class Font {
    public:
        virtual ~Font() = default;
        virtual void render_char(SDL_Renderer* renderer, SDL_Texture* texture, char c, float offset_x , float offset_y) = 0;
    };

    // TODO -  might want to create a wrapper for Textures (maybe)
    class JoustFont : public Font {
    public:
        static JoustFont& instance(SDL_Renderer* r = nullptr) {
            static JoustFont jf{r, "assets/fonts/JoustFont.png"};
            return jf;
        }        
        void render_char(SDL_Renderer* renderer, SDL_Texture* texture, char c, float offset_x , float offset_y) override;

        ~JoustFont();

        // we don't want any of these
        JoustFont(const JoustFont&) = delete;
        JoustFont(JoustFont&&) = delete;
        JoustFont& operator=(const JoustFont&) = delete;
        JoustFont& operator=(JoustFont&&) = delete;
    private:
        JoustFont(SDL_Renderer* renderer, const char* font_path);
        void setup_map(void);

        SDL_Texture* m_font_map;
        std::unordered_map<char, int> m_char_to_idx;
    };
}