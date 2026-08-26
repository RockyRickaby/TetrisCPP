
#include "texture.hpp"

static inline SDL_Surface* load_surface_from_png(const char* filepath);
static inline SDL_Texture* texture_from_surface(SDL_Renderer* renderer, SDL_Surface* surface, const char* filepath);

namespace TEngine {
    TextureWrapper load_texture(const std::filesystem::path& texture_path, SDL_ScaleMode scale_mode, SDL_Renderer* renderer) {
        const std::string str_path = texture_path.string();
        const char* cpath = str_path.c_str();

        SDL_Surface* img = load_surface_from_png(cpath);

        SDL_Texture* texture = texture_from_surface(renderer, img, cpath);
        SDL_SetTextureScaleMode(texture, scale_mode);
        SDL_DestroySurface(img);

        return TextureWrapper{texture};
    }

    TextureWrapper load_texture_with_colorkey(const std::filesystem::path& texture_path, SDL_ScaleMode scale_mode, SDL_Color color_key, SDL_Renderer* renderer) {
        const std::string str_path = texture_path.string();
        const char* cpath = str_path.c_str();

        SDL_Surface* img = load_surface_from_png(cpath);
        std::uint32_t key = SDL_MapRGB(SDL_GetPixelFormatDetails(img->format), nullptr, color_key.r, color_key.g, color_key.b);
        SDL_SetSurfaceColorKey(img, true, key);

        SDL_Texture* texture = texture_from_surface(renderer, img, cpath);
        SDL_SetTextureScaleMode(texture, scale_mode);
        SDL_DestroySurface(img);

        return TextureWrapper(texture);
    }

}

static inline SDL_Surface* load_surface_from_png(const char* filepath) {
    SDL_Surface* img = SDL_LoadPNG(filepath);
    if (!img) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "SpriteAtlas(const char*): could not load font image in path '%s': %s",
            filepath, SDL_GetError()
        );
        std::abort();
    }
    return img;
}

static inline SDL_Texture* texture_from_surface(SDL_Renderer* renderer, SDL_Surface* surface, const char* filepath) {
    SDL_Texture* img_txt = SDL_CreateTextureFromSurface(renderer, surface);
    if (!img_txt) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "SpriteAtlas(const char*): surface loaded from '%s', but could not create texture from it: %s",
            filepath, SDL_GetError()
        );
        std::abort();
    }
    return img_txt;
}