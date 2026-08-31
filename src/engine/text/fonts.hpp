#pragma once

#include <SDL3/SDL.h>
#include "../sprites/sprites.hpp"

namespace TEngine::Text {
    class BitmapFont {
    public:
        virtual ~BitmapFont() = default;

        BitmapFont() = default;
        // move pre-configured atlas 
        BitmapFont(Sprites::SpriteAtlas&& atlas, int tile_size);
        BitmapFont(BitmapFont&& other) noexcept;
 
        int tile_size() { return m_tile_size; }
        SDL_Renderer* get_renderer(void) { return m_atlas.get_renderer(); }
        void render_char(char c, float offset_x, float offset_y, float scale);
        
        static BitmapFont load_font(const std::filesystem::path& filepath, int tile_size, SDL_Renderer* renderer, SDL_ScaleMode scale_mode, Color key);
        
        BitmapFont& operator=(BitmapFont other);
        friend void swap(BitmapFont& fst, BitmapFont& snd) {
            using std::swap;

            swap(fst.m_atlas, snd.m_atlas);
            swap(fst.m_tile_size, snd.m_tile_size);
        }
    private:
        Sprites::SpriteAtlas m_atlas{};
        int m_tile_size = 0;
    };
}