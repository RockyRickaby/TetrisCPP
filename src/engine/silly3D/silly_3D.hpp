#pragma once

#include <tuple>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <tuple>
#include <SDL3/SDL.h> // already included from "tengine.hpp"
#include "../tengine.hpp"
#include "../texture_wrapper.hpp"

// NOTE: WARNING: NOT TO BE USED FOR ANY SERIOUS 3D IN ANY CIRCUMSTANCE
namespace TEngine::Silly3D {
    struct SillyMaterial {
        TextureWrapper texture;
    };
    // file format: basically a .obj.
    // only the following lines are read by this class: usemtl, v, vn, vt and f. Other lines are ignored.
    // Note: usemtl should point to a PNG texture, not a material name.
    // rendering code taken mostly from here https://github.com/tsoding/formula
    struct SillyModel {
        struct GeometryFNArgs {
            const Vec3& position_v;
            const Vec3& rotation_v;
            Vec3 light_pos = {0, 4, 1};
            float light_ambient = 0.55f;
            Color color = {255, 255, 255, 255};
            int window_w = 0;
            int window_h = 0;
            int render_w = 0;
            int render_h = 0;
        };
        struct WireframeFNArgs {
            const Vec3& position_v;
            const Vec3& rotation_v;
            Color color = {255, 255, 255, 255};
            int window_w = 0;
            int window_h = 0;
            int render_w = 0;
            int render_h = 0;
            bool jitter = false;
            bool backface_cull = false;
        };

        std::string name;
        std::vector<Vec3> vertices;
        std::vector<Vec3> vert_normals;
        std::vector<Vec2> text_uv;
        std::vector<std::tuple<int,int,int>> indices;
        // TextureWrapper texture;
        SillyMaterial* material;
        int face_len;
        
        // renderer argument is only used if needed to load a texture
        SillyModel(const std::string& name, const std::vector<Vec3>& verts, const std::vector<int>& faces, int faces_len);
        SillyModel(
            const std::string& name,
            std::vector<Vec3>&& verts,
            std::vector<Vec3>&& normals,
            std::vector<Vec2>&& texture_uv,
            std::vector<std::tuple<int, int, int>>&& indices,
            // TextureWrapper&& texture,
            SillyMaterial* material,
            int faces_len
        );

        SillyMaterial* get_material() const { return material; }
        int get_face_length() const { return face_len; }

        void draw(WireframeFNArgs args, SDL_Renderer* renderer) const;
        void draw_fill(GeometryFNArgs args, SDL_Renderer* renderer) const;
    };

    // better for static geometry
    class SillyInstance3DCached {
    public:
        SillyInstance3DCached() : 
            model{},
            renderer{},
            position{},
            rotation{},
            color{},
            cull_wireframe(),
            jitter{},
            fill{},
            m_window_w{},
            m_window_h{},
            m_needs_update{}
        {}

        SillyInstance3DCached(SillyModel* model, Vec3 pos, Vec3 rot, Color c, SDL_Renderer* renderer, int win_w, int win_h) :
            model{model},
            renderer{renderer},
            position{pos},
            rotation{rot},
            color{c},
            cull_wireframe(false),
            jitter{false},
            fill{false},
            m_window_w{win_w},
            m_window_h{win_h},
            m_render_w{win_w < win_h ? win_w : win_h},
            m_render_h{m_render_w},
            m_needs_update{true}
        {}

        SillyModel* model;
        SDL_Renderer* renderer;
        Vec3 position;
        Vec3 rotation;
        Color color;
        
        bool cull_wireframe;
        bool jitter;
        bool fill;

        void update_window(int win_w, int win_h) {
            m_window_w = win_w;
            m_window_h = win_h;

            m_render_w = win_w < win_h ? win_w : win_h;
            m_render_h = m_render_w;

            m_needs_update = true;
        }
        // calling this funcion will trigger a regeneration of the whole geometry
        // on the next draw call
        void move(const Vec3& move_vec);
        void set_position(const Vec3& pos);
        // euler angles.
        // calling this funcion will trigger a regeneration of the whole geometry
        // on the next draw call
        void rotate_x(float r);
        void rotate_y(float r);
        void rotate_z(float r);
        void draw_instance(const Vec3& light_pos, float light_ambient);
        void draw_wireframe();
        void draw_geometry(const Vec3& light_pos, float light_ambient);
    private:
        std::vector<SDL_Vertex> m_geometry;
        std::vector<SDL_FPoint> m_wireframe;
        std::vector<int> m_geo_indices;
        // resolution of the window/logical presentation
        int m_window_w;
        int m_window_h;
        int m_render_w;
        int m_render_h;

        bool m_needs_update;
    };

    // better for dynamic geometry (in terms of memory usage, not necessarily performance)
    struct SillyInstance3D {
        SillyModel* model;
        SillyMaterial* material;
        Vec3 position;
        Vec3 rotation;
        Color color;

        int m_window_w = 0;
        int m_window_h = 0;
        int m_render_w = 0;
        int m_render_h = 0;
        
        bool cull_wireframe = false;
        bool jitter = false;
        bool fill = false;

        SDL_Renderer* renderer = nullptr;

        void set_jitter_range(float min, float max);
        // euler angles.
        void rotate_x(float r);
        void rotate_y(float r);
        void rotate_z(float r);
        void draw_instance(const Vec3& light_pos, float light_ambient);
        void draw_wireframe();
        void draw_geometry(const Vec3& light_pos, float light_ambient);

        void update_window(int win_w, int win_h) {
            m_window_w = win_w;
            m_window_h = win_h;

            m_render_w = win_w < win_h ? win_w : win_h;
            m_render_h = m_render_w;
        }
    };

    class SillyAssetManager {
    public:
        SillyAssetManager(SDL_Renderer* r) : m_renderer{r} {}

        SillyModel& load_model(const std::filesystem::path& model_path);
        SillyMaterial& load_material(const std::filesystem::path& material_path);

        SillyModel& get_model(const std::string& model);
        SillyMaterial& get_material(const std::string& material);
        
        SillyInstance3D instance_from(const std::string& model, int win_w, int win_h);
        SillyInstance3DCached cached_instance_from(const std::string& model, int win_w, int win_h);
    private:
        std::unordered_map<std::string, SillyModel> m_models;
        std::unordered_map<std::string, SillyMaterial> m_materials;
        SDL_Renderer* m_renderer;
    };

    // class SillyWorld {
    // public:
    //     SillyWorld() = default;

    //     Vec3 light_pos;
    //     float light_radius = 0;
    //     float light_ambient = 0;

    //     // constructs the SillyInstance3D directly on the internal data structure
    //     template<typename ...Args>
    //     void emplace_instance(Args&&... args) { m_instances.emplace_back(std::forward<Args...>(args)...); }
    //     // clones the given instance
    //     void push_instance(const SillyInstance3D& instance) { m_instances.push_back(instance); }
    //     // linear operation - O(n)
    //     void remove_instance(const std::string& name) { std::erase_if(m_instances, [&name](const auto& instance){ return instance.name == name; }); }
        
    //     // draws all instances
    //     // void draw_instances(void) { for (auto& inst : m_instances) inst.draw_instance(); }
    //     void reserve(size_t reserve_size) { m_instances.reserve(reserve_size); }

    //     std::vector<SillyInstance3D>::iterator begin() { return m_instances.begin(); }
    //     std::vector<SillyInstance3D>::iterator end() { return m_instances.end(); }
    // private:
    //     std::vector<SillyInstance3D> m_instances;
    // };
}