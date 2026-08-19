#pragma once

#include <SDL3/SDL.h>
#include "../sprites/sprites.hpp"

namespace TEngine::Text {
    class BitmapFont {
    public:
        virtual ~BitmapFont() = default;
        virtual int tile_size() = 0;
        virtual SDL_Renderer* get_renderer(void) = 0;
        virtual void render_char(char c, float offset_x, float offset_y, float scale) = 0;
    };

    class JoustFont final : public BitmapFont {
    public:
        JoustFont(const std::filesystem::path& font_path, int tile_size, SDL_Renderer* renderer, Color color_key);
        constexpr int tile_size() { return 8; }
        SDL_Renderer* get_renderer(void) { return m_atlas.get_renderer(); }
        void render_char(char c, float offset_x, float offset_y, float scale);
    private:
        void setup_atlas(void);

        int m_tile_size;
        Sprites::SpriteAtlas m_atlas;
    };
}