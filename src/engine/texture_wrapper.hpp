#pragma once

#include <memory>
// #include <iostream>
#include <SDL3/SDL.h>

namespace TEngine {
    // Helper class for automatically managing the lifetime of SDL_Texture instances.
    // wrapper around a unique_ptr with a custom deleter.
    // movable, but non-copyable (I don't want to deal with texture copying. just use a pointer (or a reference)).
    class TextureWrapper {
    private:
        // custom deleter so we can just use a flipping std::unique_ptr (no shared pointers!!!)
        struct TextureDeleter {
            void operator()(SDL_Texture* texture) {
                // std::cerr << "calling delete on TextureWrapper\n";
                SDL_DestroyTexture(texture);
            }
        };
    public:
        TextureWrapper() : m_texture{nullptr} {}
        TextureWrapper(SDL_Texture* texture) : m_texture{texture} {}

        SDL_Texture* get() const noexcept { return m_texture.get(); }
        SDL_Texture* operator->() const noexcept { return m_texture.get(); }
        // not sure if this one makes sense.
        // written for consistency with the -> operator.
        // avoid copying the structure, as it may (or may not) slice the actual texture data.
        SDL_Texture& operator*() const noexcept { return *m_texture; }

        // returns the dimenions (width and height) of the texture in pixels
        std::pair<int, int> dimensions() const noexcept {
            float w, h;
            if (!SDL_GetTextureSize(m_texture.get(), &w, &h)) {
                return { -1, -1 };
            }
            int iw = static_cast<int>(w);
            int ih = static_cast<int>(h);
            return { iw, ih };
        }
    private:
        std::unique_ptr<SDL_Texture, TextureDeleter> m_texture;
    };

    // I probably wouldn't use this because of the shared_ptr reference counting.
    // besides, textures would ideally have a very specific lifetime (from start to the end
    // of the app or maybe of some particular event (like a game stage), by which point no class should be
    // dependent on it)
    class SharedTextureWrapper {
    private:
        // custom deleter so we can just use a flipping std::unique_ptr (no shared pointers!!!)
        struct TextureDeleter {
            void operator()(SDL_Texture* texture) {
                // std::cerr << "calling delete on SharedTextureWrapper\n";
                SDL_DestroyTexture(texture);
            }
        };
    public:
        SharedTextureWrapper() : m_texture{nullptr} {}
        SharedTextureWrapper(SDL_Texture* texture) : m_texture{texture, TextureDeleter{}} {}
        
        SDL_Texture* get() const noexcept { return m_texture.get(); }
        SDL_Texture* operator->() const noexcept { return m_texture.get(); }
        // not sure if this one makes sense.
        // written for consistency with the -> operator.
        // avoid copying the structure, as it may (or may not) slice the actual texture data.
        SDL_Texture& operator*() const noexcept { return *m_texture; }
        
        // returns the dimenions (width and height) of the texture in pixels
        std::pair<int, int> dimensions() const noexcept {
            float w, h;
            if (!SDL_GetTextureSize(m_texture.get(), &w, &h)) {
                return { -1, -1 };
            }
            int iw = static_cast<int>(w);
            int ih = static_cast<int>(h);
            return { iw, ih };
        }
    private:
        std::shared_ptr<SDL_Texture> m_texture;
    };
}