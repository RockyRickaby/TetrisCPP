#pragma once

#include <tuple>
#include <vector>
#include <filesystem>
#include <tuple>
#include <random>
#include <SDL3/SDL.h> // already included from "tengine.hpp"
#include "../tengine.hpp"
#include "../texture_wrapper.hpp"

namespace TEngine::Silly3D {
    // file format: basically a .obj (and .mtl for the material (only diffuse (map_Kd) colors supported))
    // rendering code taken mostly from here https://github.com/tsoding/formula
    class SillyModel {
    public:
        SillyModel(const std::filesystem::path& sillymodel_path, SDL_Renderer* renderer, Color c, int window_width, int window_height);
        SillyModel(const std::vector<Vec3>& verts, const std::vector<int>& faces, int faces_len, SDL_Renderer* renderer, Color c, int window_width, int window_height);

        // calling this funcion will trigger a regeneration of the whole geometry
        // on the next draw call
        void move(const Vec3& move_vec);
        // euler angles.
        // calling this funcion will trigger a regeneration of the whole geometry
        // on the next draw call
        void rotate_x(float r);
        void rotate_y(float r);
        void rotate_z(float r);

        void set_backface_culling(bool enable) { m_cull = enable; }
        void set_jitter(bool enable, float min = 0.0f, float max = 0.0f) { m_jitter = enable; m_dist = std::uniform_real_distribution<float>{min, max}; }

        void draw() const;
        void draw_fill() const;

        Vec3 position;
        Color color;
    private:
        std::mt19937 m_random_engine;
        std::uniform_real_distribution<float> m_dist;
        // these two might not be needed
        std::vector<Vec3> m_vertices;
        std::vector<Vec2> m_text_uv;
        std::vector<std::pair<int,int>> m_geo_indices;
        Vec3 m_rotation_v; // does not prevent gimbal locks
        TextureWrapper m_texture;
        SDL_Renderer* m_renderer;
        // float m_rotation;
        int m_face_len;

        int m_win_w;
        int m_win_h;

        bool m_cull;
        bool m_jitter;
    };

    class SillyInstance3D {
    public:
        std::string name;
        SillyModel* model;
        Vec3 position;
        Vec3 rotation;
        Color color;
        int win_w;
        int win_h;
        
        bool cull_wireframe = false;
        bool jitter = false;

        void draw_instance() const;
    };

    class SillyWorld {
    public:
        SillyWorld() = default;

        template<typename ...Args>
        void emplace_instance(Args&&... args) { m_instances.emplace_back(std::forward(args)...); }
        void push_instance(const SillyInstance3D& instance) { m_instances.push_back(instance); }
        
        void draw_instances(void) { for (const auto& inst : m_instances) inst.draw_instance(); }
        
        std::vector<SillyInstance3D>::iterator begin() { return m_instances.begin(); }
        std::vector<SillyInstance3D>::iterator end() { return m_instances.end(); }
    private:
        std::vector<SillyInstance3D> m_instances;
    };
}