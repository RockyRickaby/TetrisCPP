#pragma once

// #include <memory>
#include <unordered_map>
#include <utility>
#include <SDL3/SDL.h>

namespace TEngine::Sprites {
    struct Sprite {
        SDL_Renderer* renderer;
        SDL_Texture* texture;
    };

    // concrete class. only loads pngs right now
    class SpriteAtlas final {
    public:
        // SpriteAtlas stores the pointer to the renderer, but only uses it to
        // use SDL's render functions.
        // This will load the image at filepath as a texture. The texture will be
        // managed automatically by this class.
        // Only call this constructor after initializing SDL
        SpriteAtlas(const char* filepath, int tile_width, int tile_height, SDL_Renderer* renderer, SDL_ScaleMode scale_mode);
        ~SpriteAtlas();

        int tile_size() { return m_tile_size; }
        int tile_width() { return m_tile_w; }
        int tile_height() { return m_tile_h; }
        SDL_Renderer* get_renderer(void) { return m_renderer; }

        // stores an offset pair associated with a sprite_id.
        // overwrites previously defined offsets if any are present.
        // offsets should be relative to the top-left corner of the atlas' subdivisions
        void insert_offsets(int sprite_id, int offset_x, int offset_y);
        // this function expects to be drawing to a texture of size equal to the size of the tiles
        void render_texture(int sprite_id, float offset_x, float offset_y);
        // offsets correspond to screen offsets in pixels.
        // scale refers to the scale of the sprite relative to its size
        void render(int sprite_id, float offset_x, float offset_y, float scale);
    private:
        void texture_setup(const char* filepath, SDL_ScaleMode scale_mode);
        int m_tile_w;
        int m_tile_h;
        int m_tile_size;
        SDL_Texture* m_texture;
        SDL_Renderer* m_renderer;
        std::unordered_map<int, std::pair<int, int>> m_id_to_offsets;
    };

    namespace SpriteRenderer {
        // Sprite instances should be rendered with the TEngine::Text::draw_bitmap_text() function.
        // Must be freed by calling the TEngine::Text::destroy_bitmap_text() function
        Sprite make_sprite(SpriteAtlas* atlas, int sprite_id);
        void draw_from_atlas(SpriteAtlas* atlas, int sprite_id, float x, float y, float scale);
        // void draw_float(SpriteAtlas* f, float num, float x, float y, float scale);
    }
    // destroys the text's internal data
    void destroy_sprite(Sprite& text);
    void draw_sprite(const Sprite& text, const SDL_FRect& target_rect);
}