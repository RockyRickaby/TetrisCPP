#pragma once

#include <filesystem>

#include "tetris_pieces.hpp"
#include "../engine/tengine.hpp"
#include "../engine/silly3D/silly_3D.hpp"

namespace Tetris {
    struct Piece3D {
        TEngine::Silly3D::SillyInstance3D instance;
        TEngine::Vec3 default_pos;
        TEngine::Vec3 default_rot;
        TEngine::Color default_color;
        Tetrimino::Type type;

        void draw(TEngine::Vec3 light_pos, float amb_light);
        void draw_wireframe();
        void draw_filled(TEngine::Vec3 light_pos, float amb_light);
        void reset();
    };

    Piece3D load_piece3d_tri(const std::filesystem::path& pieces_folder, TEngine::Silly3D::SillyAssetManager& asset_loader, Tetrimino::Type type);
    Piece3D load_piece3d_quad(const std::filesystem::path& pieces_folder, TEngine::Silly3D::SillyAssetManager& asset_loader, Tetrimino::Type type);
}