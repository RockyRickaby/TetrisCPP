#include <SDL3/SDL_surface.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <SDL3/SDL.h>

#include "fonts.hpp"

namespace TEngine::Text {
    void BitmapFont::render_char(char c, float offset_x, float offset_y, float scale) {
        m_atlas.render(static_cast<int>(c), offset_x, offset_y, scale);
    }

    BitmapFont::BitmapFont(Sprites::SpriteAtlas&& atlas, int tile_size) :
        m_atlas{std::move(atlas)},
        m_tile_size{tile_size}
    {}

    BitmapFont::BitmapFont(BitmapFont&& other) noexcept :
        m_atlas(std::move(other.m_atlas)),
        m_tile_size(other.m_tile_size)
    {}

    BitmapFont BitmapFont::load_font(const std::filesystem::path& filepath, int tile_size, SDL_Renderer* renderer, SDL_ScaleMode scale_mode, Color key) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading bitmap font %s.", filepath.string().c_str());
        std::ifstream fs;
        fs.open(filepath);
        if (!fs.is_open()) {
            throw std::runtime_error(std::string{"could not find path: "} + filepath.string());
        }

        std::string line;
        Sprites::SpriteAtlas spr;
        while (std::getline(fs, line)) {
            std::istringstream iss{line};
            std::string data;

            iss >> data;
            if (data == "atlas") {
                iss >> data;
                spr = Sprites::SpriteAtlas(filepath.parent_path() / data, tile_size, tile_size, renderer, scale_mode, key);
            } else if (data == "m") {
                char ch;
                int x;
                int y;
                iss >> ch;
                iss >> x;
                iss >> y;
                spr.insert_offsets(static_cast<int>(ch), x, y);
            } else if (data == "b") {
                char ch = ' ';
                int x;
                int y;
                iss >> x;
                iss >> y;
                spr.insert_offsets(static_cast<int>(ch), x, y);
            } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ignoring line %s.", line.c_str());
            }
        }
        return {std::move(spr), tile_size};
    }

    BitmapFont& BitmapFont::operator=(BitmapFont other) {
        swap(*this, other);
        return *this;
    }
}