#include <SDL3/SDL_log.h>
#include <filesystem>

#include "sprites.hpp"
#include "../texture.hpp"

namespace TEngine::Sprites {
    SpriteAtlas::SpriteAtlas(const std::filesystem::path& filepath, int tile_width, int tile_height, SDL_Renderer* renderer, SDL_ScaleMode scale_mode) :
        m_tile_w{tile_width},
        m_tile_h{tile_height},
        m_tile_size{tile_width * tile_height},
        m_renderer{renderer},
        m_texture{load_texture(filepath, scale_mode, renderer)}
    {}

    SpriteAtlas::SpriteAtlas(const std::filesystem::path& filepath, int tile_width, int tile_height, SDL_Renderer* renderer, SDL_ScaleMode scale_mode, Color ckey) :
        m_tile_w{tile_width},
        m_tile_h{tile_height},
        m_tile_size{tile_width * tile_height},
        m_renderer{renderer},
        m_texture{load_texture_with_colorkey(filepath, scale_mode, {ckey.r, ckey.g, ckey.b, ckey.a}, renderer)}
    {}

    void SpriteAtlas::insert_offsets(int sprite_id, int offset_x, int offset_y) {
        if (m_texture->w < offset_x * m_tile_w || m_texture->h < offset_y * m_tile_h) {
            //std::clog << "[WARN]: (SpriteAtlas::insert_offsets) " << std::endl;
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "offsets to sprite point to out of bounds area: x = %d, y = %d.", offset_x, offset_y);
        }
        m_id_to_offsets.insert_or_assign(sprite_id, std::make_pair(offset_x, offset_y));
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
        SDL_RenderTexture(m_renderer, m_texture.get(), &source, &dest);
        // SDL_RenderTexture(m_renderer, m_texture, &source, &dest);
    }

    namespace SpriteRenderer {
        Sprite make_sprite(SpriteAtlas* atlas, int sprite_id) {
            SDL_Texture* char_texture = SDL_CreateTexture(atlas->get_renderer(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, atlas->tile_size(), atlas->tile_size());
            SDL_SetTextureScaleMode(char_texture, SDL_SCALEMODE_NEAREST);
            SDL_SetRenderTarget(atlas->get_renderer(), char_texture);
            SDL_SetRenderDrawColor(atlas->get_renderer(), 0, 0, 0, SDL_ALPHA_TRANSPARENT);
            SDL_RenderClear(atlas->get_renderer());
            // atlas->render_texture(sprite_id, 0, 0);
            atlas->render(sprite_id, 0, 0, 1);
            SDL_SetRenderTarget(atlas->get_renderer(), nullptr);
            return Sprite{atlas->get_renderer(), char_texture};
        }
        
        // this one might be better than make_sprite in terms of resource usage
        // since it just draws straight to the screen
        void draw_from_atlas(SpriteAtlas* atlas, int sprite_id, float x, float y, float scale) {
            atlas->render(sprite_id, x, y, scale);
        }
    }
}