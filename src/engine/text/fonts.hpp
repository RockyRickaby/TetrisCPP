#pragma once

#include <SDL3/SDL.h>
#include "../sprites/sprites.hpp"

namespace TEngine::Text {
    using TextFlipMode = Sprites::SpriteFlipMode;

    // BitmapFont assumes all characters are in tiles of same width and height, where width == height.
    class BitmapFont {
    public:
        static BitmapFont load_font(const std::filesystem::path& filepath, int tile_size, SDL_Renderer* renderer, SDL_ScaleMode scale_mode, Color key);

        BitmapFont() = default;
        // move pre-configured atlas 
        // BitmapFont(Sprites::SpriteAtlas&& atlas, int tile_size);
        BitmapFont(BitmapFont&& other) noexcept;
 
        int tile_size() { return m_atlas.tile_width(); }
        SDL_Renderer* get_renderer(void) { return m_atlas.get_renderer(); }
        Sprites::SpriteAtlas& get_texture_atlas(void) { return m_atlas; }
        void draw_char(char c, float offset_x, float offset_y, float scale, float angle = 0, TextFlipMode flipmode = {});
        
        BitmapFont& operator=(BitmapFont other);
        friend void swap(BitmapFont& fst, BitmapFont& snd) {
            using std::swap;

            swap(fst.m_atlas, snd.m_atlas);
            // swap(fst.m_tile_size, snd.m_tile_size);
        }
    private:
        Sprites::SpriteAtlas m_atlas{};
    };
}