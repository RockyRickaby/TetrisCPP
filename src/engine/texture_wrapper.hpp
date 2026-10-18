#pragma once

#include <memory>
#include <iostream>
#include <SDL3/SDL.h>

namespace TEngine {
    struct __TextureDeleter {
        void operator()(SDL_Texture* texture) const {
            // std::cerr << "calling delete on TextureDeleter\n";
            SDL_DestroyTexture(texture);
        }
    };

    struct __SurfaceDeleter {
        void operator()(SDL_Surface* surface) const {
            // std::cerr << "calling delete on SurfaceDeleter\n";
            SDL_DestroySurface(surface);
        }
    };

    // Helper class for automatically managing the lifetime of SDL_Texture instances.
    // wrapper around a unique_ptr with a custom deleter.
    // movable, but non-copyable (I don't want to deal with texture copying. just use a pointer (or a reference)).
    class TextureWrapper : public std::unique_ptr<SDL_Texture, __TextureDeleter> {
    public:
        // using UniqueWrapper::UniqueWrapper;
        // using std::unique_ptr<SDL_Texture, __TextureDeleter>::unique_ptr;
        TextureWrapper() noexcept : unique_ptr{} {}
        TextureWrapper(SDL_Texture* t) noexcept : unique_ptr{t} {}

        // returns the dimenions (width and height) of the texture in pixels
        std::pair<int, int> dimensions() const noexcept {
            float w, h;
            if (!SDL_GetTextureSize(get(), &w, &h)) {
                return { -1, -1 };
            }
            int iw = static_cast<int>(w);
            int ih = static_cast<int>(h);
            return { iw, ih };
        }
    };

    // I probably wouldn't use this because of the shared_ptr reference counting.
    // besides, textures would ideally have a very specific lifetime (from start to the end
    // of the app or maybe of some particular event (like a game stage), by which point no class should be
    // dependent on it)
    class SharedTextureWrapper : public std::shared_ptr<SDL_Texture> {
    public:
        SharedTextureWrapper() noexcept : shared_ptr{} {}
        SharedTextureWrapper(SDL_Texture* t) noexcept : shared_ptr{t, __TextureDeleter{}} {}
        
        // returns the dimenions (width and height) of the texture in pixels
        std::pair<int, int> dimensions() const noexcept {
            float w, h;
            if (!SDL_GetTextureSize(get(), &w, &h)) {
                return { -1, -1 };
            }
            int iw = static_cast<int>(w);
            int ih = static_cast<int>(h);
            return { iw, ih };
        }
    };

    // these could be typedefs
    class SurfaceWrapper : public std::unique_ptr<SDL_Surface, __SurfaceDeleter> {
    public:
        SurfaceWrapper() noexcept : unique_ptr{} {}
        SurfaceWrapper(SDL_Surface* t) noexcept : unique_ptr{t} {}

        std::pair<int, int> dimensions() const noexcept {
            return { get()->w, get()->h };
        }
    };

    class SharedSurfaceWrapper : public std::shared_ptr<SDL_Surface> {
    public:
        SharedSurfaceWrapper() noexcept : shared_ptr{} {}
        SharedSurfaceWrapper(SDL_Surface* t) noexcept : shared_ptr{t, __SurfaceDeleter{}} {}

        std::pair<int, int> dimensions() const noexcept {
            return { get()->w, get()->h };
        }
    };
}