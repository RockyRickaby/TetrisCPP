#pragma once

// #include <memory>
#include <unordered_map>
#include <utility>
#include <filesystem>
#include <SDL3/SDL.h>
#include "../tengine.hpp"
#include "../texture_wrapper.hpp"

namespace TEngine::Sprites {
    struct Sprite {
        SDL_Renderer* renderer{nullptr};
        TextureWrapper texture{nullptr};
    };

    // concrete class. only loads pngs right now.
    // may be moved. may not be copied (I don't wanna handle copying textures at all!!)
    class SpriteAtlas final {
    public:
        SpriteAtlas() = default;
        // SpriteAtlas stores the pointer to the renderer, but only uses it to
        // use SDL's render functions.
        // This will load the image at filepath as a texture. The texture will be
        // managed automatically by this class.
        // Filepath must be a c string.
        // Only call this constructor after initializing SDL.
        SpriteAtlas(const std::filesystem::path& filepath, int tile_width, int tile_height, SDL_Renderer* renderer, SDL_ScaleMode scale_mode);
        // SpriteAtlas stores the pointer to the renderer, but only uses it to
        // use SDL's render functions.
        // This will load the image at filepath as a texture and apply the color key to it to apply transparency.
        // The texture will be managed automatically by this class.
        // Filepath must be a c string.
        // Only call this constructor after initializing SDL.
        SpriteAtlas(const std::filesystem::path& filepath, int tile_width, int tile_height, SDL_Renderer* renderer, SDL_ScaleMode scale_mode, Color ckey);

        int tile_size() { return m_tile_size; }
        int tile_width() { return m_tile_w; }
        int tile_height() { return m_tile_h; }
        SDL_Renderer* get_renderer(void) { return m_renderer; }

        // stores an offset pair associated with a sprite_id.
        // overwrites previously defined offsets if any are present.
        // offsets should be relative to the top-left corner of the atlas' subdivisions
        void insert_offsets(int sprite_id, int offset_x, int offset_y);
        // offsets correspond to screen position in pixels.
        // scale refers to the scale of the sprite relative to its size
        void render(int sprite_id, float offset_x, float offset_y, float scale);

    private:
        int m_tile_w =- 0;
        int m_tile_h =- 0;
        int m_tile_size = 0;
        SDL_Renderer* m_renderer = nullptr;
        TextureWrapper m_texture{nullptr};
        std::unordered_map<int, std::pair<int, int>> m_id_to_offsets;
    };

    namespace SpriteRenderer {
        // Sprite instances should be rendered with the TEngine::Text::draw_bitmap_text() function.
        // Must be freed by calling the TEngine::Text::destroy_bitmap_text() function
        Sprite make_sprite(SpriteAtlas* atlas, int sprite_id);
        void draw_from_atlas(SpriteAtlas* atlas, int sprite_id, float x, float y, float scale);
        // void draw_float(SpriteAtlas* f, float num, float x, float y, float scale);
    }

    // // TODO - improve this
    // class SpriteEntity {
    // public:
    //     template<typename ...indices>
    //     SpriteEntity(SpriteAtlas* atlas, int quads, Vec2 pos, float scale, indices... args) :
    //         pos{pos},
    //         m_indices{args...},
    //         m_atlas{atlas},
    //         scale{scale},
    //         m_quads{quads},
    //         m_qidx{0}
    //     {}
        
    //     Vec2 pos;
    //     void draw() {
    //         int off = 0;
    //         for (auto it = m_indices.begin() + m_qidx; it != m_indices.begin() + m_quads; ++it) {
    //             std::cout << "whaa " << *it << std::endl;
    //             SpriteRenderer::draw_from_atlas(m_atlas, *it, pos.x + m_atlas->tile_width() * scale * off, pos.y + scale, scale);
    //             off += 1;
    //         }
    //     }

    //     void set_sprite_id_offset(int offset) { m_qidx = offset; }
    // private:
    //     std::vector<int> m_indices;
    //     SpriteAtlas* m_atlas;
    //     float scale = 0;
    //     int m_quads = 0;
    //     int m_qidx = 0;
    // };

    inline void draw_sprite(const Sprite& sprite, const SDL_FRect& target_rect) {
        SDL_RenderTexture(sprite.renderer, sprite.texture.get(), nullptr, &target_rect);
    }
}