#pragma once

// #include <memory>
#include <SDL3/SDL_surface.h>
#include <unordered_map>
#include <utility>
#include <filesystem>
#include <SDL3/SDL.h>
#include "../tengine.hpp"
#include "../texture_wrapper.hpp"

namespace TEngine::Sprites {
    // struct Sprite {
    //     SDL_Renderer* renderer{nullptr};
    //     TextureWrapper texture{nullptr};
    // };

    enum class SpriteFlipMode {
        None = SDL_FLIP_NONE,
        Horizontal = SDL_FLIP_HORIZONTAL,
        Vertical = SDL_FLIP_VERTICAL,
        Both = SDL_FLIP_HORIZONTAL_AND_VERTICAL
    };

    // concrete class. only loads pngs right now.
    // may be moved. may not be copied (I don't wanna handle copying textures at all!!)
    class SpriteAtlas final {
    public:
        using spriteid_type = int;

        SpriteAtlas() = default;
        SpriteAtlas(TextureWrapper&& texture, int tile_width, int tile_height, SDL_Renderer* renderer);
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

        int tile_size() const { return m_tile_size; }
        int tile_width() const { return m_tile_w; }
        int tile_height() const { return m_tile_h; }
        int atlas_width() const { return m_atlas_w; }
        int atlas_height() const { return m_atlas_h; }
        SDL_Renderer* get_renderer(void) const { return m_renderer; }
        SDL_Texture* get_texture(void) const { return m_texture.get(); }

        // stores an offset pair associated with a sprite_id.
        // overwrites previously defined offsets if any are present.
        // offsets should be relative to the top-left corner of the atlas' subdivisions
        void insert_offsets(spriteid_type sprite_id, int offset_x, int offset_y);
        // offsets correspond to screen position in pixels.
        // scale refers to the scale of the sprite relative to its size
        void draw(spriteid_type sprite_id, float offset_x, float offset_y, float scale, float angle = 0, SpriteFlipMode flipmode = {});
    private:
        int m_tile_w = 0;
        int m_tile_h = 0;
        int m_tile_size = 0;
        int m_atlas_w = 0;
        int m_atlas_h = 0;
        SDL_Renderer* m_renderer = nullptr;
        TextureWrapper m_texture{nullptr};
        std::unordered_map<spriteid_type, std::pair<int, int>> m_id_to_offsets;
    };

    struct KeyFrame {
        Countdown duration;
        SpriteAtlas::spriteid_type sprite_id;
    };

    struct SpriteAnimation {
        std::vector<KeyFrame> frames;
        Countdown duration;
    };

    class SpriteAnimator {
    public: 
    private:
        std::vector<SpriteAnimation> m_animations;
    };

    // TODO - improve this
    class SpriteEntity {
    public:
        SpriteEntity(SpriteAtlas* atlas) :
            rotation{0},
            scale{0},
            position{0,0},
            flipmode{SpriteFlipMode::None},
            m_animator{nullptr},
            m_atlas{atlas}
        {}
        
        float rotation;
        float scale;
        Vec2 position{};
        SpriteFlipMode flipmode;

        void draw_frame(SpriteAtlas::spriteid_type frame);
        void draw();
    private:
        SpriteAnimator* m_animator;
        SpriteAtlas* m_atlas;
    };
    
    // useful just for not having to manage atlases manually. entities are on our own, though
    class SpriteManager {
    public:
        SpriteManager() : m_renderer{nullptr} {}
        SpriteManager(SDL_Renderer* renderer) : m_renderer{renderer} {}
        SpriteAtlas& load_atlas(const std::filesystem::path& sprite_atlas_path, const std::string& name, int tile_width, int tile_height, SDL_ScaleMode scale_mode);
        SpriteAtlas& get_atlas(const std::string& atlas_name);
        SpriteEntity entity_for_atlas(const std::string& sprite_atlas);

        bool set_renderer(SDL_Renderer* renderer) {
            bool was_null = false;
            if (m_renderer == nullptr) {
                m_renderer = renderer;
                was_null = true;
            }
            return was_null;
        }
    private:
        std::unordered_map<std::string, SpriteAtlas> m_atlases;
        SDL_Renderer* m_renderer;
    };

    // inline void draw_sprite(const Sprite& sprite, const SDL_FRect& target_rect) {
    //     SDL_RenderTexture(sprite.renderer, sprite.texture.get(), nullptr, &target_rect);
    // }
}