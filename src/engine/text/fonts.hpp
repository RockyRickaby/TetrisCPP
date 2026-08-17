#pragma once

#include <unordered_map>
#include <SDL3/SDL.h>

namespace TEngine::Text::BitmapFonts {
    // this font interface is to be used with Font objects that
    class Font {
    public:
        virtual ~Font() = default;
        virtual int tile_size() = 0;
        virtual SDL_Renderer* get_renderer(void) = 0;
        virtual void render_char_texture(char c, float offset_x , float offset_y) = 0;
        virtual void render_char_renderer(char c, float offset_x, float offset_y, float scale) = 0;
    };

    class JoustFont final : public Font {
    public:
        JoustFont(SDL_Renderer* renderer, const char* font_path);
        ~JoustFont();

        SDL_Renderer* get_renderer(void) override { return m_renderer; }
        void render_char_texture(char c, float offset_x , float offset_y) override;
        void render_char_renderer(char c, float offset_x, float offset_y, float scale) override;
        constexpr int tile_size() override { return 8; }

        // we don't want any of these. just use a pointer
        JoustFont(const JoustFont&) = delete;
        JoustFont(JoustFont&&) = delete;
        JoustFont& operator=(const JoustFont&) = delete;
        JoustFont& operator=(JoustFont&&) = delete;

    private:
        void setup_map(void);

        SDL_Renderer* m_renderer;
        SDL_Texture* m_font_map;
        std::unordered_map<char, int> m_char_to_idx;
    };

    // TODO - just for funsies, let's try creating a bitmap font that uses only ints to store its data
}