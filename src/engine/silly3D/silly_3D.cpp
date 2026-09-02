#include <SDL3/SDL.h>
#include <SDL3/SDL_oldnames.h>
#include <algorithm>
#include <cassert>
#include <charconv>
#include <iostream>
#include <fstream>
#include <array>
#include <stdexcept>
#include <cmath>
#include <string>
#include <string_view>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>

#include "silly_3D.hpp"
#include "../tengine.hpp"
#include "../texture.hpp"

static inline TEngine::Vec3 rotate_vx(TEngine::Vec3 vec, float r);
static inline TEngine::Vec3 rotate_vy(TEngine::Vec3 vec, float r);
static inline TEngine::Vec3 rotate_vz(TEngine::Vec3 vec, float r);
static inline TEngine::Vec3 rotate_all(TEngine::Vec3 vec, TEngine::Vec3 rot_vec);
static inline TEngine::Vec3 translate_v(TEngine::Vec3 vec, TEngine::Vec3 position);
static inline TEngine::Vec2 project_v(TEngine::Vec3 vec);
static inline TEngine::Vec2 to_screen_coords(TEngine::Vec2 p, int win_w, int win_h, int rend_w, int rend_h);

static inline TEngine::Vec3 jitter_vec(float min, float max, bool jitter_individually = true);
static inline SDL_FColor get_flat_color(const std::vector<TEngine::Vec3>& face, TEngine::Vec3 face_normal, const SDL_FColor& t_color, TEngine::Vec3 light, float amb);
static inline SDL_FColor get_flat_specular(const std::vector<TEngine::Vec3>& face, TEngine::Vec3 face_normal, const SDL_FColor& t_color, TEngine::Vec3 light, float amb);
static inline SDL_FColor get_gouraud_color(TEngine::Vec3 vert, TEngine::Vec3 vert_normal, const SDL_FColor& t_color, TEngine::Vec3 light, float amb);
static inline SDL_FColor get_gouraud_specular(TEngine::Vec3 vert, TEngine::Vec3 vert_normal, const SDL_FColor& t_color, TEngine::Vec3 light, float amb);

namespace TEngine::Silly3D {
    template<typename FN>
    concept WireframeLambda = requires(FN func, std::vector<Vec2>& points_vec, int face_len) {
        { func(points_vec, face_len) } -> std::same_as<void>;
    };

    // @fn is called whenever a non-culled face is ready to be used, during the generation loop
    template<WireframeLambda T>
    static inline void generate_wireframe_with_accept(const SillyModel* model, SillyModel::WireframeFNArgs args, T fn) {
        std::vector<Vec3> verts;
        std::vector<Vec2> points;
        verts.resize(model->face_len);
        points.resize(model->face_len);
        for (auto it = model->indices.cbegin(); it != model->indices.cend(); it += model->face_len) {
            bool skip = false;
            for (int i = 0; i < model->face_len; ++i) {
                verts[i] = translate_v(rotate_all(model->vertices[std::get<0>(*(it + i))], args.rotation_v), args.position_v);
                skip = skip || verts[i].z < 0;
            }
            // face is behind camera. skip generation to prevent visual bugs
            if (skip) continue;
            Vec3 vv1 = verts[1] - verts[0];
            Vec3 vv2 = verts[2] - verts[1];

            // cache these?
            Vec3 normal = vv1.cross(vv2).normalize();
            float align = normal.dot(verts[0].normalize());
            // back-face culling
            if (align < 0 || !args.backface_cull) {
                for (int i = 0; i < model->face_len; ++i) {
                    if (args.jitter) {
                        points[i] = to_screen_coords(project_v(verts[i] + jitter_vec(-0.025f, 0.025f, true)), args.window_w, args.window_h, args.render_w, args.render_h);
                        // points[i] = to_screen_coords(project_v(verts[i]), args.window_w, args.window_h, args.render_w, args.render_h);
                    } else {
                        points[i] = to_screen_coords(project_v(verts[i]), args.window_w, args.window_h, args.render_w, args.render_h);
                    }
                    // out_points.emplace_back(points[i].x, points[i].y);
                }
                fn(points, model->face_len);
            }
        }
    }

    template<typename FN>
    concept GeometryLambda = requires(FN func, std::vector<SDL_Vertex>& geometry, std::vector<int>& indices) {
        { func(geometry, indices) } -> std::same_as<void>;
    };

    // @fn is called at the end of the generation, after all faces have been culled and sorted
    template<GeometryLambda T>
    static inline void generate_geometry_with_accept(const SillyModel* model, SillyModel::GeometryFNArgs args, T fn) {
        assert(model->face_len == 3 && "currently, can't fill geometry if the faces aren't made of triangles");
        
        struct __convenience {
            std::array<int, 3> data;
        };
        // std::vector<SDL_Vertex> geometry;
        // std::vector<int> indices;
        std::vector<__convenience> indices_c;
        int verts_idx = 0;
        std::vector<int> vert_group;
        std::vector<float> z_group;

        std::vector<Vec3> verts;
        std::vector<Vec2> points;
        std::vector<SDL_Vertex> geometry;
        verts.resize(model->face_len);
        points.resize(model->face_len);
        geometry.reserve(model->vertices.size() / 2);
        for (auto it = model->indices.cbegin(); it != model->indices.cend(); it += model->face_len) {
            bool skip = false;
            for (int i = 0; i < model->face_len; ++i) {
                verts[i] = translate_v(rotate_all(model->vertices[std::get<0>(*(it + i))], args.rotation_v), args.position_v);
                skip = skip || verts[i].z < 0;
            }
            // face is behind camera. skip generation to prevent visual bugs
            if (skip) continue;
            Vec3 vv1 = verts[1] - verts[0];
            Vec3 vv2 = verts[2] - verts[1];

            // cache these?
            Vec3 normal = vv1.cross(vv2).normalize();
            float align = normal.dot(verts[0]);
            // back-face culling
            if (align < 0) {
                SDL_FColor new_color = { args.color.r / 255.0f, args.color.g / 255.0f, args.color.b / 255.0f, args.color.a / 255.0f };
                new_color = get_flat_color(verts, normal, new_color, args.light_pos, args.light_ambient);
                __convenience ind_struct;
                for (int i = 0; i < model->face_len; ++i) {
                    points[i] = to_screen_coords(project_v(verts[i]), args.window_w, args.window_h, args.render_w, args.render_h);
                    SDL_FPoint tex_c = 
                        std::get<1>(*(it + i)) < 0 ?
                            SDL_FPoint{0, 0} : 
                            SDL_FPoint{model->text_uv[std::get<1>(*(it + i))].x, model->text_uv[std::get<1>(*(it + i))].y};
                            
                    // auto nv = std::get<2>(*(it + i)) < 0 ? normal : rotate_all(vert_normals[std::get<2>(*(it + i))], args.rotation_v);
                            
                    // new_color = get_gouraud_color(verts[i], nv, new_color, args.light_pos, args.light_ambient);
                    // might be wasteful... I think we're recreating a few vertices that were already stored
                    geometry.emplace_back(SDL_Vertex{
                        {points[i].x, points[i].y},
                        new_color,
                        tex_c
                    });
                    // indices.push_back(i + verts_idx);
                    ind_struct.data[i] = i + verts_idx;
                }
                indices_c.emplace_back(ind_struct);
                z_group.push_back((verts[0].z + verts[1].z + verts[2].z) / 3);
                vert_group.push_back(verts_idx);
                verts_idx += model->face_len;
            }
        }

        std::ranges::sort(vert_group, [&z_group](int lhs, int rhs){
            return z_group.at(lhs / 3) > z_group.at(rhs / 3);
        });

        std::vector<int> vertices;
        vertices.reserve(indices_c.size() * 3);
        for (int g : vert_group) {
            const auto& v = indices_c[g / 3];
            vertices.push_back(v.data[0]);
            vertices.push_back(v.data[1]);
            vertices.push_back(v.data[2]);
        }
        fn(geometry, vertices);
    }


    SillyModel::SillyModel(const std::string& name, const std::vector<Vec3>& verts, const std::vector<int>& faces, int faces_len) :
        name{name},
        vertices{verts},
        text_uv{},
        indices{},
        material{nullptr},
        face_len{faces_len}
    {
        assert(face_len >= 3 && "length of faces should be greater than 2 (otherwise it's just a line)");
        for (int idx : faces) {
            indices.emplace_back(idx, -1, -1);
        }
        assert(faces.size() == indices.size());
    }

    SillyModel::SillyModel(
        const std::string& name,
        std::vector<Vec3>&& verts,
        std::vector<Vec3>&& normals,
        std::vector<Vec2>&& texture_uv,
        std::vector<std::tuple<int, int, int>>&& indices,
        SillyMaterial* material,
        int faces_len
    ) :
        name{name},
        vertices{std::move(verts)},
        vert_normals{std::move(normals)},
        text_uv{std::move(texture_uv)},
        indices{std::move(indices)},
        material{material},
        face_len{faces_len}
    {}

    void SillyModel::draw(WireframeFNArgs args, SDL_Renderer* renderer) const {
        std::vector<SDL_FPoint> lines;
        lines.reserve(vertices.size());
        SDL_BlendMode b;
        SDL_GetRenderDrawBlendMode(renderer, &b);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, args.color.r, args.color.g, args.color.b, SDL_ALPHA_OPAQUE);
        generate_wireframe_with_accept(this, args,
            [renderer](auto& points, int face_len){
                for (int k = 0; k < face_len; k++) {
                    SDL_RenderLine(renderer,
                        points[k].x,
                        points[k].y,
                        points[(k + 1) % face_len].x,
                        points[(k + 1) % face_len].y
                    );
                }
            }
        );
        SDL_SetRenderDrawBlendMode(renderer, b);
    }

    void SillyModel::draw_fill(GeometryFNArgs args, SDL_Renderer* renderer) const {
        assert(face_len == 3 && "currently, can't fill geometry if the faces aren't made of triangles");
        SDL_BlendMode b;
        SDL_GetRenderDrawBlendMode(renderer, &b);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Texture* txt = material->texture.get();
        generate_geometry_with_accept(this, args,
            [renderer, txt](auto& v, auto& i){
                SDL_RenderGeometry(
                    renderer,
                    txt,
                    v.data(),
                    static_cast<int>(v.size()),
                    i.data(),// vert_group.data(),
                    i.size() // vert_group.size()
                );
            }
        );
        SDL_SetRenderDrawBlendMode(renderer, b);
    }
    





    void SillyInstance3DCached::move(const Vec3& move_vec) {
        position += move_vec;
        m_needs_update = true;
    }

    void SillyInstance3DCached::set_position(const Vec3& pos) {
        position = pos;
        m_needs_update = true;
    }

    void SillyInstance3DCached::rotate_x(float r) {
        rotation.x += r;
        if (rotation.x < 0) rotation.x += 2 * SDL_PI_F;
        else if (rotation.x >= 2 * SDL_PI_F) rotation.x -= 2 * SDL_PI_F;
        m_needs_update = true;
    }

    void SillyInstance3DCached::rotate_y(float r) {
        rotation.y += r;
        if (rotation.y < 0) rotation.y += 2 * SDL_PI_F;
        else if (rotation.y >= 2 * SDL_PI_F) rotation.y -= 2 * SDL_PI_F;
        m_needs_update = true;
    }

    void SillyInstance3DCached::rotate_z(float r) {
        rotation.z += r;
        if (rotation.z < 0) rotation.z += 2 * SDL_PI_F;
        else if (rotation.z >= 2 * SDL_PI_F) rotation.z-= 2 * SDL_PI_F;
        m_needs_update = true;
    }

    void SillyInstance3DCached::draw_instance(const Vec3& light_pos, float light_ambient) {
        if (fill) {
            draw_geometry(light_pos, light_ambient);
        } else {
            draw_wireframe();
        }
    }

    void SillyInstance3DCached::draw_wireframe() {
        if (m_needs_update) {
            m_wireframe.clear();
            auto& ps = m_wireframe;
            generate_wireframe_with_accept(model,{
                .position_v = position,
                .rotation_v = rotation,
                .color = color,
                .window_w = m_window_w,
                .window_h = m_window_h,
                .render_w = m_render_w,
                .render_h = m_render_h,
                .jitter = jitter,
                .backface_cull = cull_wireframe
            }, [&ps](auto& points, int){
                for (auto& v : points) {
                    ps.emplace_back(v.x, v.y);
                }
            });
            m_needs_update = false;
        }
        
        int face_len = model->get_face_length();
        int wire_len = static_cast<int>(m_wireframe.size());
        SDL_BlendMode b;
        SDL_GetRenderDrawBlendMode(renderer, &b);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
        for (int i = 0; i < wire_len; i += face_len) {
            for (int k = 0; k < face_len; k++) {
                SDL_RenderLine(
                    renderer,
                    m_wireframe[k + i].x,
                    m_wireframe[k + i].y,
                    m_wireframe[(k + 1) % face_len + i].x,
                    m_wireframe[(k + 1) % face_len + i].y
                );
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, b);
        // SDL_RenderLines(renderer, m_wireframe.data(), m_wireframe.size());
    }

    void SillyInstance3DCached::draw_geometry(const Vec3& light_pos, float light_ambient) {
        if (m_needs_update) {
            auto& ver = m_geometry;
            auto& ind = m_geo_indices;
            generate_geometry_with_accept(model,{
                .position_v = position,
                .rotation_v = rotation,
                .light_pos = light_pos,
                .light_ambient = light_ambient,
                .color = color,
                .window_w = m_window_w,
                .window_h = m_window_h,
                .render_w = m_render_w,
                .render_h = m_render_h
            }, [&ver, &ind](auto& v, auto& i){
                ver = std::move(v);
                ind = std::move(i);
            });
            m_needs_update = false;
        }
        SDL_BlendMode b;
        SDL_GetRenderDrawBlendMode(renderer, &b);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderGeometry(
            renderer,
            model->get_material()->texture.get(),
            m_geometry.data(),
            static_cast<int>(m_geometry.size()),
            m_geo_indices.data(),
            static_cast<int>(m_geo_indices.size())
        );
        SDL_SetRenderDrawBlendMode(renderer, b);
    }




    

    void SillyInstance3D::rotate_x(float r) {
        rotation.x += r;
        if (rotation.x < 0) rotation.x += 2 * SDL_PI_F;
        else if (rotation.x >= 2 * SDL_PI_F) rotation.x -= 2 * SDL_PI_F;
    }

    void SillyInstance3D::rotate_y(float r) {
        rotation.y += r;
        if (rotation.y < 0) rotation.y += 2 * SDL_PI_F;
        else if (rotation.y >= 2 * SDL_PI_F) rotation.y -= 2 * SDL_PI_F;
    }

    void SillyInstance3D::rotate_z(float r) {
        rotation.z += r;
        if (rotation.z < 0) rotation.z += 2 * SDL_PI_F;
        else if (rotation.z >= 2 * SDL_PI_F) rotation.z -= 2 * SDL_PI_F;
    }

    void SillyInstance3D::draw_instance(const Vec3& light_pos, float light_ambient) const {
        if (fill) {
            draw_geometry(light_pos, light_ambient);
        } else {
            draw_wireframe();
        }
    }

    void SillyInstance3D::draw_wireframe() const {
        SDL_BlendMode b;
        SDL_Renderer* r = renderer;
        SDL_GetRenderDrawBlendMode(r, &b);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
        generate_wireframe_with_accept(model, {
            .position_v = position,
            .rotation_v = rotation,
            .color = color,
            .window_w = m_window_w,
            .window_h = m_window_h,
            .render_w = m_render_w,
            .render_h = m_render_h,
            .jitter = jitter,
            .backface_cull = cull_wireframe
        }, [r](const auto& points, int face_len) -> void {
            for (int k = 0; k < face_len; k++) {
                SDL_RenderLine(
                    r,
                    points[k].x,
                    points[k].y,
                    points[(k + 1) % face_len].x,
                    points[(k + 1) % face_len].y
                );
            }
        });
        SDL_SetRenderDrawBlendMode(r, b);
        // SDL_RenderLines(r, m_wireframe.data(), m_wireframe.size());
    }

    void SillyInstance3D::draw_geometry(const Vec3& light_pos, float light_ambient) const {
        SDL_BlendMode b;
        SDL_Renderer* r = renderer;
        SDL_GetRenderDrawBlendMode(r, &b);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_Texture* texture = model->get_material()->texture.get();
        generate_geometry_with_accept(model, {
            .position_v = position,
            .rotation_v = rotation,
            .light_pos = light_pos,
            .light_ambient = light_ambient,
            .color = color,
            .window_w = m_window_w,
            .window_h = m_window_h,
            .render_w = m_render_w,
            .render_h = m_render_h
        }, [r, texture](const auto& g, const auto& v){
            SDL_RenderGeometry(
                r,
                texture,
                g.data(),
                static_cast<int>(g.size()),
                v.data(),
                static_cast<int>(v.size())
            );
        });
        SDL_SetRenderDrawBlendMode(r, b);
    }

    SillyModel& SillyAssetManager::load_model(const std::filesystem::path& model_path) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading model %s.", model_path.string().c_str());
        // throw std::runtime_error("path constructor not implemented for SillyModel yet");
        // assert(m_face_len >= 3 && "length of faces should be greater than 2 (otherwise it's just a line)");
        std::ifstream fs;
        fs.open(model_path);
        if (!fs.is_open()) {
            throw std::runtime_error(std::string{"could not find path: "} + model_path.string());
        }

        std::string line;
        int has_texture = 0;
        int has_normals = 0;
        std::string name;
        std::vector<Vec3> vertices;
        std::vector<Vec3> vert_normals;
        std::vector<Vec2> text_uv;
        std::vector<std::tuple<int,int,int>> indices;

        SillyMaterial* mat = nullptr;
        int face_len = 0;
        while(std::getline(fs, line)) {
            std::istringstream iss{line};
            std::string data;
            iss >> data;
            if (data == "mtllib") {
                iss >> data; // material file
                // texture = load_texture(model_path.parent_path() / data, SDL_SCALEMODE_LINEAR, m_renderer);
                mat = &load_material(model_path.parent_path() / data);
                has_texture = 1;
            } else if (data == "usemtl") {
                iss >> data;
                mat = &get_material(data);
            } else if (data == "o") {
                iss >> name;
            } else if (data == "v") {
                float x, y, z;
                iss >> x;
                iss >> y;
                iss >> z;
                vertices.emplace_back(x, y, z);
            } else if (data == "vt") {
                float x, y;
                iss >> x;
                iss >> y;
                text_uv.emplace_back(x, y);
                has_texture = 1;
            } else if (data == "vn") {
                float x,y,z;
                iss >> x;
                iss >> y;
                iss >> z;
                vert_normals.emplace_back(x, y, z);
                has_normals = 1;
            } else if (data == "f") {
                int faces = 0;
                while (iss >> data) {
                    std::string_view vertidx{data};
                    std::string_view texidx{vertidx.begin() + vertidx.find('/') + 1};
                    int vidx = -1;
                    int tidx = -1;
                    int nidx = -1;
                    std::from_chars(vertidx.data(), vertidx.data() + vertidx.size(), vidx);
                    if (has_texture) {
                        std::from_chars(texidx.data(), texidx.data() + texidx.size(), tidx);
                        tidx -= 1;
                    }
                    if (has_normals) {
                        std::string_view normidx{vertidx.begin() + vertidx.find_last_of('/') + 1};
                        std::from_chars(normidx.data(), normidx.data() + normidx.size(), nidx);
                        nidx -= 1;
                    }
                    // face_reader >> vidx;
                    vidx -= 1;
                    indices.emplace_back(vidx, tidx, nidx);
                    faces++;
                }
                face_len = faces;
            } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ignoring line %s.", line.c_str());
            }
        }
        m_models.emplace(std::make_pair(name, SillyModel{
            name,
            std::move(vertices),
            std::move(vert_normals),
            std::move(text_uv),
            std::move(indices),
            mat,
            face_len
        }));

        return m_models.at(name);
    }

    SillyMaterial& SillyAssetManager::load_material(const std::filesystem::path& material_path) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading material %s.", material_path.string().c_str());
        std::ifstream fs;
        fs.open(material_path);
        if (!fs.is_open()) {
            throw std::runtime_error(std::string{"could not find path: "} + material_path.string());
        }
        std::string line;
        std::string mat_name;
        TextureWrapper txt;
        while(std::getline(fs, line)) {
            std::istringstream iss{line};
            std::string data;
            iss >> data;

            if (data == "newmtl") {
                iss >> mat_name;
                if (m_materials.contains(mat_name)) {
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Material already loaded: %s.", material_path.string().c_str());
                    return m_materials[mat_name];
                }
            } else if (data == "map_Kd") {
                iss >> data;
                txt = load_texture(material_path.parent_path() / data, SDL_SCALEMODE_LINEAR, m_renderer);
            } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ignoring line %s.", line.c_str());
            }
        }

        m_materials.emplace(std::make_pair(mat_name, SillyMaterial{std::move(txt)}));
        return m_materials.at(mat_name);
    }
    
    SillyModel& SillyAssetManager::get_model(const std::string& model) {
        return m_models.at(model);
    }

    SillyMaterial&SillyAssetManager:: get_material(const std::string& material) {
        return m_materials.at(material);
    }

    SillyInstance3D SillyAssetManager::instance_from(const std::string& model, int win_w, int win_h) {
        auto& mod = m_models.at(model);
        SillyInstance3D ret = {
            .model = &mod,
            .material = mod.material,
            .color = {255,255,255,255},
            .renderer = m_renderer
        };
        ret.update_window(win_w, win_h);
        return ret;
    }

    SillyInstance3DCached SillyAssetManager::cached_instance_from(const std::string& model, int win_w, int win_h) {
        auto& mod = m_models.at(model);
        return {
            &mod,
            {}, {}, {255,255,255,255},
            m_renderer, win_w, win_h
        };
    }
}

static inline TEngine::Vec3 rotate_vx(TEngine::Vec3 vec, float r) {
    const auto c = std::cos(r);
    const auto s = std::sin(r);
    return {
        vec.x,
        vec.y * c - vec.z * s,
        vec.y * s + vec.z * c
    };
}

static inline TEngine::Vec3 rotate_vy(TEngine::Vec3 vec, float r) {
    const auto c = std::cos(r);
    const auto s = std::sin(r);
    return {
        vec.x * c - vec.z * s,
        vec.y,
        vec.x * s + vec.z * c
    };
}

static inline TEngine::Vec3 rotate_vz(TEngine::Vec3 vec, float r) {
    const auto c = std::cos(r);
    const auto s = std::sin(r);
    return {
        vec.x * c - vec.y * s,
        vec.x * s + vec.y * c,
        vec.z
    };
}

// TODO - bad. make this better
static inline TEngine::Vec3 rotate_all(TEngine::Vec3 vec, TEngine::Vec3 rot_vec) {
    return rotate_vx(
        rotate_vy(
            rotate_vz(
                vec,
                rot_vec.z
            ),
            rot_vec.y
        ),
        rot_vec.x
    );
}

static inline TEngine::Vec3 translate_v(TEngine::Vec3 vec, TEngine::Vec3 position) {
    return vec + position;
}

static inline TEngine::Vec2 project_v(TEngine::Vec3 vec) {
    return {
        vec.x / vec.z,
        vec.y / vec.z
    };
}

static inline TEngine::Vec2 to_screen_coords(TEngine::Vec2 p, int win_w, int win_h, int rend_w, int rend_h) {
    float width = static_cast<float>(win_w);
    float height = static_cast<float>(win_h);

    float r_w = static_cast<float>(rend_w);
    float r_h = static_cast<float>(rend_h);

    TEngine::Vec2 res{
        ((p.x + 1) / 2.0f) * r_w,
        (1 - (p.y + 1) / 2.0f) * r_h
    };
    return {
        (width - r_w) / 2.0f + res.x,
        (height - r_h) / 2.0f + res.y
    };
}

static inline TEngine::Vec3 jitter_vec(float min, float max, bool jitter_individually) {
    float j = TEngine::random_float(min, max);
    
    return jitter_individually ? TEngine::Vec3{
        j,
        TEngine::random_float(min, max),
        TEngine::random_float(min, max)
    } : TEngine::Vec3{
        j,
        j,
        j
    };
}

static inline SDL_FColor get_flat_color(const std::vector<TEngine::Vec3>& face, TEngine::Vec3 face_normal, const SDL_FColor& t_color, TEngine::Vec3 light, float amb) {
    // TEngine::Vec3 center = {
    //     (face[0].x + face[1].x + face[2].x) / 3,
    //     (face[0].y + face[1].y + face[2].y) / 3,
    //     (face[0].z + face[1].z + face[2].z) / 3,
    // };
    auto L = (light - face[0]).normalize();
    float dotNL = face_normal.dot(L); // angle between vecs
    // float factor = std::fmax(0, dotNL) + amb;
    float factor = dotNL + amb;

    return {
        t_color.r * factor,
        t_color.g * factor,
        t_color.b * factor,
        t_color.a
    };
}

static inline SDL_FColor get_flat_specular(const std::vector<TEngine::Vec3>& face, TEngine::Vec3 face_normal, const SDL_FColor& t_color, TEngine::Vec3 light, float amb) {
    auto& N = face_normal;
    // light = light.normalize();

    // auto L = (light - face[0]).normalize();
    auto L = (light - face[0]).normalize();
    auto V = (TEngine::Vec3{} - face[0]).normalize();

    float dotNL = N.dot(L);
    // float dotNV = N.dot(V);
    // float dotVL = V.dot(L);

    auto K = (N * -2 * dotNL + L).normalize();
    // float res = std::pow(2 * dotNL * dotNV - dotVL, 100);
    // 1 = Ks, 10 = Ns
    float res = 1 * std::pow(K.dot(V), 10);
    float factor = std::fmax(0, res);

    return {
        t_color.r * (dotNL + factor + amb),
        t_color.g * (dotNL + factor + amb),
        t_color.b * (dotNL + factor + amb),
        t_color.a
    };
}

static inline SDL_FColor get_gouraud_color(TEngine::Vec3 vert, TEngine::Vec3 vert_normal, const SDL_FColor& t_color, TEngine::Vec3 light, float amb) {
    vert_normal = vert_normal.normalize();
    // light = light.normalize();

    auto L = (light - vert).normalize();

    float dotNL = vert_normal.dot(L);
    float factor = std::fmax(0, dotNL) + amb;

    return {
        t_color.r * factor,
        t_color.g * factor,
        t_color.b * factor,
        t_color.a
    };
}

static inline SDL_FColor get_gouraud_specular(TEngine::Vec3 vert, TEngine::Vec3 N, const SDL_FColor& t_color, TEngine::Vec3 light, float amb) {
    N = N.normalize();
    // light = light.normalize();

    auto L = (light - vert).normalize();
    auto V = (TEngine::Vec3{} + vert).normalize();

    float dotNL = N.dot(L);
    float dotNV = N.dot(V);
    float dotVL = V.dot(L);

    float res = std::pow(2 * dotNL * dotNV - dotVL, 10);
    float factor = std::fmax(0, res);

    return {
        t_color.r * (dotNL + amb) + factor,
        t_color.g * (dotNL + amb) + factor,
        t_color.b * (dotNL + amb) + factor,
        t_color.a
    };
}