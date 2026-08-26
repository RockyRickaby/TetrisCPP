#pragma once

#include <filesystem>
#include <SDL3/SDL.h>
#include "texture_wrapper.hpp"

namespace TEngine {
    TextureWrapper load_texture(const std::filesystem::path& texture_path, SDL_ScaleMode scale_mode, SDL_Renderer* renderer);
    TextureWrapper load_texture_with_colorkey(const std::filesystem::path& texture_path, SDL_ScaleMode scale_mode, SDL_Color color_key, SDL_Renderer* renderer);
}