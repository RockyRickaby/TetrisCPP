#include <unordered_map>

#include "tetris_piece3d.hpp"
#include "tetris_pieces.hpp"

namespace Tetris {
    static const auto type_to_name = std::unordered_map{
        std::make_pair<Tetrimino::Type, std::string>(Tetrimino::Type::I, "Ipiece"),
        std::make_pair<Tetrimino::Type, std::string>(Tetrimino::Type::J, "Jpiece"),
        std::make_pair<Tetrimino::Type, std::string>(Tetrimino::Type::L, "Lpiece"),
        std::make_pair<Tetrimino::Type, std::string>(Tetrimino::Type::O, "Opiece"),
        std::make_pair<Tetrimino::Type, std::string>(Tetrimino::Type::S, "Spiece"),
        std::make_pair<Tetrimino::Type, std::string>(Tetrimino::Type::T, "Tpiece"),
        std::make_pair<Tetrimino::Type, std::string>(Tetrimino::Type::Z, "Zpiece")
    };

    static inline Piece3D load_piece(const std::filesystem::path& piece_path, TEngine::Silly3D::SillyAssetManager& asset_loader, Tetrimino::Type type) {
        const auto& model = asset_loader.load_model(piece_path);
        auto instance = asset_loader.instance_from(model.name, 0, 0);

        bool should_shift = type == Tetrimino::Type::O || type == Tetrimino::Type::S || type == Tetrimino::Type::Z;
        bool should_rotate = type == Tetrimino::Type::None && false;
        auto def_pos = TEngine::Vec3{0,0,4};
        auto def_rot = TEngine::Vec3{0,-SDL_PI_F / 4.0f,0};
        if (should_rotate) {
            def_rot += {0, -SDL_PI_F / 2.0f, 0};
        }
        if (should_shift) {
            def_pos += {0, 0.5f, 0};
        }
        return {
            std::move(instance),
            def_pos,
            def_rot,
            Tetrimino::DEFAULT_PIECES.at(type).get_color(),
            type
        };
    }

    void Piece3D::draw(TEngine::Vec3 light_pos, float amb_light) {
        instance.draw_instance(light_pos, amb_light);
    }

    void Piece3D::draw_wireframe() {
        instance.draw_wireframe();
    }

    void Piece3D::draw_filled(TEngine::Vec3 light_pos, float amb_light) {
        instance.draw_geometry(light_pos, amb_light);
    }

    void Piece3D::reset() {
        instance.position = default_pos;
        instance.rotation = default_rot;
        instance.color = default_color;
    }

    Piece3D load_piece3d_tri(const std::filesystem::path& pieces_folder, TEngine::Silly3D::SillyAssetManager& asset_loader, Tetrimino::Type type) {
        auto t = load_piece(pieces_folder / (type_to_name.at(type) + "Tri.obj"), asset_loader, type);
        t.instance.fill = true;
        t.instance.shading = false;
        return t;
    }

    Piece3D load_piece3d_quad(const std::filesystem::path& pieces_folder, TEngine::Silly3D::SillyAssetManager& asset_loader, Tetrimino::Type type) {
        auto t = load_piece(pieces_folder / (type_to_name.at(type) + ".obj"), asset_loader, type);
        t.instance.fill = false;
        t.instance.cull_wireframe = false;
        return t;
    }
}