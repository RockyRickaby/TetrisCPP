#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <algorithm>
#include <cassert>
#include <charconv>
#include <iostream>
#include <fstream>
#include <random>
#include <queue>
#include <array>
#include <stdexcept>
#include <cmath>
#include <string>
#include <string_view>

#include "silly_3D.hpp"
#include "../tengine.hpp"
#include "../texture.hpp"

static inline TEngine::Vec3 rotate_vx(const TEngine::Vec3& vec, float r);
static inline TEngine::Vec3 rotate_vy(const TEngine::Vec3& vec, float r);
static inline TEngine::Vec3 rotate_vz(const TEngine::Vec3& vec, float r);
static inline TEngine::Vec3 rotate_all(const TEngine::Vec3& vec, const TEngine::Vec3& rot_vec);
static inline TEngine::Vec3 translate_v(const TEngine::Vec3& vec, const TEngine::Vec3& position);
static inline TEngine::Vec2 project_v(const TEngine::Vec3& vec);
static inline TEngine::Vec2 to_screen_coords(const TEngine::Vec2& p, int w, int h);

// TODO - try calculating gouraud shading at some point (if the obj file comes with the vn vectors)
static inline SDL_FColor get_flat_color(const std::vector<TEngine::Vec3>& vert, const TEngine::Vec3& normal, const SDL_FColor& t_color, TEngine::Vec3 light, float light_r, float amb);

namespace TEngine::Silly3D {
    SillyModel::SillyModel(const std::filesystem::path& sillymodel_path, SDL_Renderer* renderer, Color c, int window_width, int window_height) :
        position{},
        color{c},
        m_random_engine(std::random_device{}()),
        m_dist{0,0},
        m_vertices{},
        m_text_uv{},
        m_geo_indices{},
        m_rotation_v{},
        m_texture{nullptr},
        m_renderer{renderer},
        // m_rotation{},
        m_face_len{},
        m_win_w{window_width},
        m_win_h{window_height},
        m_cull{true},
        m_jitter{false}
    {
        // throw std::runtime_error("path constructor not implemented for SillyModel yet");
        // assert(m_face_len >= 3 && "length of faces should be greater than 2 (otherwise it's just a line)");
        std::ifstream fs;
        fs.open(sillymodel_path);
        if (!fs.is_open()) {
            throw std::runtime_error(std::string{"could not find path: "} + sillymodel_path.string());
        }

        std::string line;
        using namespace std::string_view_literals;
        int has_texture = 0;
        while(std::getline(fs, line)) {
            std::istringstream iss{line};
            std::string data;
            iss >> data;
            if (data == "usemtl") {
                iss >> data; // texture name
                m_texture = load_texture(sillymodel_path.parent_path() / data, SDL_SCALEMODE_LINEAR, m_renderer);
                has_texture = 1;
            } else if (data == "v") {
                float x, y, z;
                iss >> x;
                iss >> y;
                iss >> z;
                m_vertices.emplace_back(x, y, z);
            } else if (data == "vt") {
                float x, y;
                iss >> x;
                iss >> y;
                m_text_uv.emplace_back(x, y);
                has_texture = 1;
            } else if (data == "vn") {
                std::cout << "ignoring 'vn' lines for now";
            } else if (data == "f") {
                int faces = 0;
                while (iss >> data) {
                    std::string_view vertidx{data};
                    std::string_view texidx{vertidx.begin() + vertidx.find('/') + 1};
                    int vidx = -1;
                    int tidx = -1;
                    std::from_chars(vertidx.data(), vertidx.data() + vertidx.size(), vidx);
                    if (has_texture) {
                        std::from_chars(texidx.data(), texidx.data() + texidx.size(), tidx);
                        tidx -= 1;
                    }
                    // face_reader >> vidx;
                    vidx -= 1;
                    m_geo_indices.emplace_back(vidx, tidx);
                    faces++;
                }
                m_face_len = faces;
            } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "ignoring line %s.", line.c_str());
            }
        }
    }

    SillyModel::SillyModel(const std::vector<Vec3>& verts, const std::vector<int>& faces, int faces_len, SDL_Renderer* renderer, Color c, int window_width, int window_height) :
        position{},
        color{c},
        m_random_engine(std::random_device{}()),
        m_dist{0,0},
        m_vertices{verts},
        m_text_uv{},
        m_geo_indices{},
        m_rotation_v{},
        m_texture{nullptr},
        m_renderer{renderer},
        // m_rotation{},
        m_face_len{faces_len},
        m_win_w{window_width},
        m_win_h{window_height},
        m_cull{true},
        m_jitter{false}
    {
        assert(m_face_len >= 3 && "length of faces should be greater than 2 (otherwise it's just a line)");
        for (int idx : faces) {
            m_geo_indices.emplace_back(idx, -1);
        }
        assert(faces.size() == m_geo_indices.size());
    }

    void SillyModel::move(const Vec3& move_vec) {
        position += move_vec;
    }

    void SillyModel::rotate_x(float r) {
        m_rotation_v.x += r;
        if (m_rotation_v.x < 0) m_rotation_v.x += 360;
        else if (m_rotation_v.x >= 360) m_rotation_v.x -= 360;
    }

    void SillyModel::rotate_y(float r) {
        m_rotation_v.y += r;
        if (m_rotation_v.y < 0) m_rotation_v.y += 360;
        else if (m_rotation_v.y >= 360) m_rotation_v.y -= 360;
    }

    void SillyModel::rotate_z(float r) {
        m_rotation_v.z += r;
        if (m_rotation_v.z < 0) m_rotation_v.z += 360;
        else if (m_rotation_v.z >= 360) m_rotation_v.y -= 360;
    }

    void SillyModel::draw() const {
        std::vector<Vec3> verts;
        std::vector<Vec2> points;
        verts.resize(m_face_len);
        points.resize(m_face_len);
        SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
        for (auto it = m_geo_indices.cbegin(); it != m_geo_indices.cend(); it += m_face_len) {
            for (int i = 0; i < m_face_len; ++i) {
                verts[i] = translate_v(rotate_all(m_vertices[(it + i)->first], m_rotation_v), position);
            }

            Vec3 vv1 = verts[1] - verts[0];
            Vec3 vv2 = verts[2] - verts[1];

            // cache these?
            Vec3 normal = vv1.cross(vv2).normalize();
            float align = normal.dot(verts[0]);
            // back-face culling
            if (align < 0 || !m_cull) {
                for (int i = 0; i < m_face_len; ++i) {
                    if (m_jitter) {
                        // points[i] = to_screen_coords(project_v(verts[i] + Vec3{m_dist(m_random_engine), m_dist(m_random_engine)}), m_win_w, m_win_h);
                        points[i] = to_screen_coords(project_v(verts[i]), m_win_w, m_win_h);
                    } else {
                        points[i] = to_screen_coords(project_v(verts[i]), m_win_w, m_win_h);
                    }
                }
                for (int i = 0; i < m_face_len; ++i) {
                    SDL_RenderLine(
                        m_renderer,
                        points[i].x,
                        points[i].y,
                        points[(i + 1) % m_face_len].x,
                        points[(i + 1) % m_face_len].y
                    );
                }
            }
        }
    }

    // TODO - implement some version of the painter's algorithm
    void SillyModel::draw_fill() const {
        assert(m_face_len == 3 && "currently, can't fill geometry if the faces aren't made of triangles");
        
        struct __convenience {
            int data[3];
        };
        std::vector<SDL_Vertex> geometry;
        std::vector<int> indices;
        std::vector<__convenience> indices_c;
        int verts_idx = 0;
        std::vector<int> vert_group;
        std::vector<float> z_group;

        std::vector<Vec3> verts;
        std::vector<Vec2> points;
        verts.resize(m_face_len);
        points.resize(m_face_len);
        for (auto it = m_geo_indices.cbegin(); it != m_geo_indices.cend(); it += m_face_len) {
            for (int i = 0; i < m_face_len; ++i) {
                verts[i] = translate_v(rotate_all(m_vertices[(it + i)->first], m_rotation_v), position);
            }

            Vec3 vv1 = verts[1] - verts[0];
            Vec3 vv2 = verts[2] - verts[1];

            // cache these?
            Vec3 normal = vv1.cross(vv2).normalize();
            float align = normal.dot(verts[0]);
            // back-face culling
            if (align < 0) {
                SDL_FColor new_color = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
                new_color = get_flat_color(verts, normal, new_color, Vec3{-1, 4, 1}, 15, 0.55);
                __convenience ind_struct;
                for (int i = 0; i < m_face_len; ++i) {
                    points[i] = to_screen_coords(project_v(verts[i]), m_win_w, m_win_h);
                    SDL_FPoint tex_c = 
                        (it + i)->second < 0 ?
                            SDL_FPoint{0, 0} : 
                            SDL_FPoint{m_text_uv[(it + i)->second].x, m_text_uv[(it + i)->second].y};
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
                verts_idx += m_face_len;
            }
        }
        std::ranges::sort(vert_group, [&z_group](int lhs, int rhs){
            return z_group.at(lhs / 3) > z_group.at(rhs / 3);
        });

        indices.reserve(indices_c.size() * 3);
        for (int g : vert_group) {
            const auto& v = indices_c[g / 3];
            indices.push_back(v.data[0]);
            indices.push_back(v.data[1]);
            indices.push_back(v.data[2]);
        }
        SDL_RenderGeometry(
            m_renderer,
            m_texture.get(),
            geometry.data(),
            static_cast<int>(geometry.size()),
            indices.data(),// vert_group.data(),
            indices.size() // vert_group.size()
        );
    }
}

static inline TEngine::Vec3 rotate_vx(const TEngine::Vec3& vec, float r) {
    const auto c = std::cos(r);
    const auto s = std::sin(r);
    return {
        vec.x,
        vec.y * c - vec.z * s,
        vec.y * s + vec.z * c
    };
}

static inline TEngine::Vec3 rotate_vy(const TEngine::Vec3& vec, float r) {
    const auto c = std::cos(r);
    const auto s = std::sin(r);
    return {
        vec.x * c - vec.z * s,
        vec.y,
        vec.x * s + vec.z * c
    };
}

static inline TEngine::Vec3 rotate_vz(const TEngine::Vec3& vec, float r) {
    const auto c = std::cos(r);
    const auto s = std::sin(r);
    return {
        vec.x * c - vec.y * s,
        vec.x * s + vec.y * c,
        vec.z
    };
}

static inline TEngine::Vec3 rotate_all(const TEngine::Vec3& vec, const TEngine::Vec3& rot_vec) {
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

static inline TEngine::Vec3 translate_v(const TEngine::Vec3& vec, const TEngine::Vec3& position) {
    return vec + position;
}

static inline TEngine::Vec2 project_v(const TEngine::Vec3& vec) {
    return {
        vec.x / vec.z,
        vec.y / vec.z
    };
}

static inline TEngine::Vec2 to_screen_coords(const TEngine::Vec2& p, int w, int h) {
    float width = static_cast<float>(w);
    float height = static_cast<float>(h);
    return {
        ((p.x + 1) / 2.0f) * width,
        (1 - (p.y + 1) / 2.0f) * height
    };
}

static inline SDL_FColor get_flat_color(const std::vector<TEngine::Vec3>& vert, const TEngine::Vec3& normal, const SDL_FColor& t_color, TEngine::Vec3 light, float light_r, float amb) {
    TEngine::Vec3 center = {
        (vert[0].x + vert[1].x + vert[2].x) / 3,
        (vert[0].y + vert[1].y + vert[2].y) / 3,
        (vert[0].z + vert[1].z + vert[2].z) / 3,
    };
    light.normalize();
    center.normalize();
    TEngine::Vec3 light_vec = (center - light).normalize();

    float dist = light_vec.length();
    float intense = light.dot(normal); // angle between vecs
    // intense = intense < 0 ? 0 : intense;
    if (dist > light_r) {
        intense = 0;
    }

    float factor = intense + amb;
    if (factor > 1) factor = 1;
    return {
        t_color.r * factor,
        t_color.g * factor,
        t_color.b * factor,
        t_color.a
    };
}