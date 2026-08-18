#include <cstdlib>
#include <cstdint>
#include <iostream>
#include "sprite_renderer.hpp"

namespace TEngine::Sprites {
    SpriteAtlas::SpriteAtlas(const char* filepath, int tile_width, int tile_height, SDL_Renderer* renderer, SDL_ScaleMode scale_mode) :
        m_tile_w{tile_width},
        m_tile_h{tile_height},
        m_tile_size{tile_width * tile_height},
        m_renderer{renderer}
    {
        texture_setup(filepath, scale_mode);
    }

    SpriteAtlas::~SpriteAtlas() {
        SDL_DestroyTexture(m_texture);
    }

    void SpriteAtlas::insert_offsets(int sprite_id, int offset_x, int offset_y) {
        if (m_texture->w < offset_x * m_tile_w || m_texture->h < offset_y * m_tile_h) {
            //std::clog << "[WARN]: (SpriteAtlas::insert_offsets) " << std::endl;
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "offsets to sprite point to out of bounds area: x = %d, y = %d.", offset_x, offset_y);
        }
        m_id_to_offsets.insert_or_assign(sprite_id, std::make_pair(offset_x, offset_y));
    }

    void SpriteAtlas::texture_setup(const char* filepath, SDL_ScaleMode scale_mode) {
        SDL_Surface* img = SDL_LoadPNG(filepath);
        if (!img) {
            std::clog << "SpriteAtlas(const char*): could not load font image\n";
            std::clog << SDL_GetError();
            std::abort();
        }
        // std::uint32_t key = SDL_MapRGB(SDL_GetPixelFormatDetails(img->format), nullptr, 0, 0, 0);
        // SDL_SetSurfaceColorKey(img, true, key);
        SDL_Texture* img_txt = SDL_CreateTextureFromSurface(m_renderer, img);
        if (!img_txt) {
            std::clog << "SpriteAtlas(const char*): could not create texture from image\n";
            std::clog << SDL_GetError() << '\n';
            std::abort();
        }
        SDL_SetTextureScaleMode(img_txt, scale_mode);
        m_texture = img_txt;
        SDL_DestroySurface(img);
    }

    void SpriteAtlas::render_texture(int sprite_id, float offset_x, float offset_y) {
        auto [x, y] = m_id_to_offsets.at(sprite_id);
        SDL_FRect source = {
            .x = static_cast<float>(x) * m_tile_w,
            .y = static_cast<float>(y) * m_tile_h,
            .w = static_cast<float>(m_tile_w),
            .h = static_cast<float>(m_tile_h)
        };
        SDL_FRect dest = {
            .x = offset_x * m_tile_w,
            .y = offset_y * m_tile_h,
            .w = static_cast<float>(m_tile_w),
            .h = static_cast<float>(m_tile_h)
        };
        SDL_RenderTexture(m_renderer, m_texture, &source, &dest);
    }

    void SpriteAtlas::render(int sprite_id, float offset_x, float offset_y, float scale) {
        auto [x, y] = m_id_to_offsets.at(sprite_id);
        SDL_FRect source = {
            .x = static_cast<float>(x) * m_tile_w,
            .y = static_cast<float>(y) * m_tile_h,
            .w = static_cast<float>(m_tile_w),
            .h = static_cast<float>(m_tile_h)
        };
        SDL_FRect dest = {
            .x = offset_x,
            .y = offset_y,
            .w = static_cast<float>(m_tile_w) * scale,
            .h = static_cast<float>(m_tile_h) * scale
        };
        SDL_RenderTexture(m_renderer, m_texture, &source, &dest);
    }

    namespace SpriteRenderer {
        Sprite make_sprite(SpriteAtlas* atlas, int sprite_id) {
            SDL_Texture* char_texture = SDL_CreateTexture(atlas->get_renderer(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, atlas->tile_size(), atlas->tile_size());
            SDL_SetTextureScaleMode(char_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(atlas->get_renderer(), char_texture);
            SDL_SetRenderDrawColor(atlas->get_renderer(), 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(atlas->get_renderer());
            atlas->render_texture(sprite_id, 0, 0);
            SDL_SetRenderTarget(atlas->get_renderer(), nullptr);
            return Sprite{atlas->get_renderer(), char_texture};
        }
        
        // this one might be better than make_bitmap_text_ch in terms of resource usage
        // since it just draws straight to the screen
        void draw_from_atlas(SpriteAtlas* atlas, int sprite_id, float x, float y, float scale) {
            atlas->render(sprite_id, x, y, scale);
        }
    }

    void destroy_sprite(Sprite& sprite) {
        SDL_DestroyTexture(sprite.texture);
    }

    void draw_sprite(const Sprite& sprite, const SDL_FRect& target_rect) {
        SDL_RenderTexture(sprite.renderer, sprite.texture, nullptr, &target_rect);
    }
}