#include <SDL3/SDL.h>
#include <filesystem>
#include <format>

#include "sprites.hpp"
#include "../texture.hpp"

namespace TEngine::Sprites {
    SpriteAtlas::SpriteAtlas(TextureWrapper&& texture, int tile_width, int tile_height, SDL_Renderer* renderer) :
        m_tile_w{tile_width},
        m_tile_h{tile_height},
        m_tile_size{tile_width * tile_height},
        m_renderer{renderer},
        m_texture{std::move(texture)}
    {
        auto [w, h] =  m_texture.dimensions();
        m_atlas_w = w;
        m_atlas_h = h;
    }

    SpriteAtlas::SpriteAtlas(const std::filesystem::path& filepath, int tile_width, int tile_height, SDL_Renderer* renderer, SDL_ScaleMode scale_mode) :
        m_tile_w{tile_width},
        m_tile_h{tile_height},
        m_tile_size{tile_width * tile_height},
        m_renderer{renderer},
        m_texture{load_texture(filepath, scale_mode, renderer)}
    {
        auto [w, h] = m_texture.dimensions();
        m_atlas_w = w;
        m_atlas_h = h;
    }

    SpriteAtlas::SpriteAtlas(const std::filesystem::path& filepath, int tile_width, int tile_height, SDL_Renderer* renderer, SDL_ScaleMode scale_mode, Color ckey) :
        m_tile_w{tile_width},
        m_tile_h{tile_height},
        m_tile_size{tile_width * tile_height},
        m_renderer{renderer},
        m_texture{load_texture_with_colorkey(filepath, scale_mode, {ckey.r, ckey.g, ckey.b, ckey.a}, renderer)}
    {
        auto [w, h] = m_texture.dimensions();
        m_atlas_w = w;
        m_atlas_h = h;
    }

    void SpriteAtlas::insert_offsets(int sprite_id, int offset_x, int offset_y) {
        // not a fatal error, just a mistake
        if (m_atlas_w < offset_x * m_tile_w || m_atlas_h < offset_y * m_tile_h) {
            //std::clog << "[WARN]: (SpriteAtlas::insert_offsets) " << std::endl;
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "offsets to sprite point to out of bounds area: x = %d, y = %d.", offset_x, offset_y);
        }
        m_id_to_offsets.insert_or_assign(sprite_id, std::make_pair(offset_x, offset_y));
    }

    void SpriteAtlas::draw(int sprite_id, float offset_x, float offset_y, float scale, float angle, SpriteFlipMode flipmode) {
        auto [x, y] = m_id_to_offsets.at(sprite_id);
        SDL_FRect source = {
            .x = static_cast<float>(x) * m_tile_w,
            .y = static_cast<float>(y) * m_tile_h,
            .w = static_cast<float>(m_tile_w),
            .h = static_cast<float>(m_tile_h)
        };
        const float x_offset_fix_to_use_when_using_rotations_when_it_is_slightly_off_centered_for_no_apparent_reason
            // = -1.0f * scale;
            = 0.0f * scale;
        SDL_FRect dest = {
            .x = offset_x + x_offset_fix_to_use_when_using_rotations_when_it_is_slightly_off_centered_for_no_apparent_reason,
            .y = offset_y,
            .w = static_cast<float>(m_tile_w) * scale,
            .h = static_cast<float>(m_tile_h) * scale
        };
        SDL_FlipMode flip = static_cast<SDL_FlipMode>(flipmode);
        SDL_RenderTextureRotated(
            m_renderer,
            m_texture.get(),
            &source, &dest,
            angle ? ((180.0f * angle) / SDL_PI_F) : 0,
            NULL, flip
        );
        // SDL_RenderTexture(m_renderer, m_texture.get(), &source, &dest);
    }

    void SpriteEntity::draw_frame(SpriteAtlas::spriteid_type frame) {
        m_atlas->draw(frame, position.x, position.y, scale, rotation, flipmode);
    }

    void SpriteEntity::draw() {
        throw std::runtime_error(std::format("{}: not implemented", __PRETTY_FUNCTION__));
    }


    SpriteAtlas& SpriteManager::load_atlas(
        const std::filesystem::path& sprite_atlas_path,
        const std::string& name,
        int tile_width, int tile_height,
        SDL_ScaleMode scale_mode
    ) {
        if (m_atlases.contains(name)) {
            return m_atlases.at(name);
        }
        m_atlases.emplace(name, SpriteAtlas{sprite_atlas_path, tile_width, tile_height, m_renderer, scale_mode});
        return m_atlases.at(name);
    }

    SpriteAtlas& SpriteManager::get_atlas(const std::string& atlas_name) {
        return m_atlases.at(atlas_name);
    }

    SpriteEntity SpriteManager::entity_for_atlas(const std::string& sprite_atlas) {
        return {
            &m_atlases.at(sprite_atlas)
        };
    }

}