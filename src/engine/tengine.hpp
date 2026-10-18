#pragma once

#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <cmath>
#include <cstdint>
#include <format>
#include <iostream>
#include <SDL3/SDL.h>
#include <random>
#include <type_traits>

// TODO - separate this into different headers. Have tengine.hpp include them (core header)
namespace TEngine {
    struct AppMetadata {
        const char* name;
        const char* version;
        const char* identifier;
    };

    struct Window {
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        std::string window_name;
        int width = 0;
        int height = 0;
        int window_flags = 0;
        int logical_width = -1;
        int logical_height = -1;
        SDL_RendererLogicalPresentation logical_window_mode = SDL_LOGICAL_PRESENTATION_DISABLED;
        bool use_logical_window_size = false;
        bool vsync = false;
    };

    bool init(const AppMetadata& metadata);
    bool init_window(Window& window);
    void end_window(Window& window);

    // Constructor should be called after initializing SDL
    // Calculates the delta time between frames
    class Time {
    public:
        Time();
        double delta_time(void);
    private:
        Uint64 m_freq;
        Uint64 time_last;
    };

    struct OneShotTimer {
        double time = 0;
        bool done(double delta_t) {
            time -= delta_t;
            if (time <= 0) { time = 0; return true; }
            return false;
        }
    };

    // Countdown timer. Time is assumed to be in seconds
    class Countdown {
    public:
        // easir to just make this public than have a setter
        bool autoreset;

        Countdown(double count, bool autoreset = false);
        bool done(double delta_t);
        void set_countdown_time(double time);
        double get_countdown_time(void) { return m_time_delta; }
        double get_time_left(void) { return m_timer.time; }
        void reset(void);
    private:
        OneShotTimer m_timer;
        double m_time_delta;
    };

    class Random {
    public:
        Random() = default;
        Random(std::mt19937::result_type seed) : eng{seed} {};

        int get_int(int min, int max) { return std::uniform_int_distribution<int>{min, max}(eng); }
        float get_float(float min, float max) { return std::uniform_real_distribution<float>{min, max}(eng); }

        template<typename N> requires(std::is_arithmetic_v<N>)
        N get_number(N min, N max) {
            if constexpr (std::is_floating_point_v<N>) {
                return get_float(min, max);
            } else {
                return get_int(min, max);
            }
        }
    private:
        std::mt19937 eng{std::random_device{}()};
    };

    int random_int(int min, int max);
    float random_float(float min, float max);

    template<typename N> requires(std::is_arithmetic_v<N>)
    N random_number(N min, N max) {
        if constexpr (std::is_floating_point_v<N>) {
            return random_float(min, max);
        } else {
            return random_int(min, max);
        }
    }

    // NOTE: already available in SDL as SDL_Color
    // Components are unsigned 8 bit integers (very lightweight!!)
    struct Color {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 0;

        bool operator==(const Color other) const;
        bool operator!=(const Color other) const;

        friend std::ostream& operator<<(std::ostream& output, const Color& v);
    };

    // Takes up 16 bytes (darn...).
    // includes some methods for incrementing each member.
    // this is a struct, so the values may be modified wherever, whenever, however desired
    // (even if it means breaking the format)
    struct ColorHSB {
        float hue = 0; // [0, 360)
        float sat = 0; // [0, 1]
        float bri = 0; // [0, 1]
        float alpha = 0; // [0, 1]

        void increment_hue(float hue_inc);
        void increment_sat(float sat_inc);
        void increment_bri(float bri_inc);
        void increment_alpha(float alpha_inc);
        bool operator==(const ColorHSB& other) const;
        bool operator!=(const ColorHSB& other) const;

        friend std::ostream& operator<<(std::ostream& output, const ColorHSB& v);
    };

    // using ColorHSV = ColorHSB;
 
    struct Vec2 {
        float x = 0;
        float y = 0;

        constexpr float length() const { return std::sqrt(x * x + y * y); }
        constexpr float dot(const Vec2 other) const { return (x * other.x) + (y * other.y); }
        constexpr float cross2D(const Vec2 other) const { return (x * other.y) - (y * other.x); }

        constexpr Vec2 normalize() const {
            float len = length();
            return {
                x / len,
                y / len
            };
        }
        constexpr Vec2 rotate(float angle) const {
            auto c = std::cos(angle);
            auto s = std::sin(angle);
            return {
                x * c - y * s,
                x * s + y * c
            };
        }

        constexpr friend Vec2 operator+(const Vec2 lhs, const Vec2 rhs) {
            float x1 = lhs.x + rhs.x;
            float y1 = lhs.y + rhs.y;
            return Vec2{x1, y1};
        }

        constexpr friend Vec2 operator*(const Vec2 vec, float scalar) {
            float x1 = vec.x * scalar;
            float y1 = vec.y * scalar;
            return Vec2{x1, y1};
        }

        constexpr friend Vec2 operator*(float scalar, const Vec2 vec) {
            return vec * scalar;
        }

        constexpr friend Vec2 operator-(const Vec2 lhs, const Vec2 rhs) {
            float x1 = lhs.x - rhs.x;
            float y1 = lhs.y - rhs.y;
            return Vec2{x1, y1};
        }

        constexpr Vec2& operator+=(const Vec2 other) {
            x += other.x;
            y += other.y;
            return *this;
        }

        constexpr Vec2& operator-=(const Vec2 other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        constexpr Vec2& operator*=(float scalar) {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        constexpr friend bool operator==(const Vec2 lhs, const Vec2 rhs) {
            return lhs.x == rhs.x
                && lhs.y == rhs.y;
        }

        constexpr friend bool operator!=(const Vec2 lhs, const Vec2 rhs) {
            // return lhs.x != rhs.x || lhs.y != rhs.y;
            return !(lhs == rhs);
        }

        friend std::ostream& operator<<(std::ostream& output, const Vec2& v);
    };

    struct Vec3 {
        float x = 0;
        float y = 0;
        float z = 0;

        constexpr float length() const { return std::sqrt(x * x + y * y + z * z); }
        constexpr float dot(const Vec3 other) const {
            return
                (x * other.x) +
                (y * other.y) +
                (z * other.z);
        }

        constexpr Vec3 normalize() const {
            float len = length();
            return {
                x / len,
                y / len,
                z / len
            };
        }
        
        constexpr Vec3 cross(const Vec3 other) const {
            return {
                (y * other.z) - (z * other.y),
                (z * other.x) - (x * other.z),
                (x * other.y) - (y * other.x)
            };
        }

        constexpr friend Vec3 operator+(const Vec3 lhs, const Vec3 rhs) {
            float x1 = lhs.x + rhs.x;
            float y1 = lhs.y + rhs.y;
            float z1 = lhs.z + rhs.z;
            return Vec3{x1, y1, z1};
        }

        constexpr friend Vec3 operator*(const Vec3 vec, float scalar) {
            float x1 = vec.x * scalar;
            float y1 = vec.y * scalar;
            float z1 = vec.z * scalar;
            return Vec3{x1, y1, z1};
        }

        constexpr friend Vec3 operator*(float scalar, const Vec3 vec) {
            return vec * scalar;
        }

        constexpr friend Vec3 operator-(const Vec3 lhs, const Vec3 rhs) {
            float x1 = lhs.x - rhs.x;
            float y1 = lhs.y - rhs.y;
            float z1 = lhs.z - rhs.z;
            return Vec3{x1, y1, z1};
        }

        constexpr Vec3& operator+=(const Vec3 other) {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        constexpr Vec3& operator-=(const Vec3 other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }

        constexpr Vec3& operator*=(float scalar) {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        constexpr friend bool operator==(const Vec3 lhs, const Vec3 rhs) {
            return lhs.x == rhs.x
                && lhs.y == rhs.y
                && lhs.z == rhs.z;
        }

        constexpr friend bool operator!=(const Vec3 lhs, const Vec3 rhs) {
            return !(lhs == rhs) ;
        }

        friend std::ostream& operator<<(std::ostream& output, const Vec3& v);
    };

    // TODO - move these to TEngine::Math, probably
    // ... in a new file called tengine_math.hpp
    // template<typename T>
    // requires(requires(T a, float t){
    //     { a + a } -> std::same_as<T>;
    //     { t * a } -> std::same_as<T>;
    //     { a * t } -> std::same_as<T>;
    // })
    // T __lerp_values(T a, T b, float t) {
    //     t = std::min(std::max(0.0f, t), 1.0f);
    //     return (1 - t) * a + b * t;  
    // }

    inline float lerpf(float a, float b, float t) {
        t = std::min(std::max(0.0f, t), 1.0f);
        return (1 - t) * a + b * t;  
    }
    
    inline double lerpd(double a, double b, float t) {
        t = std::min(std::max(0.0f, t), 1.0f);
        return (1 - t) * a + b * t;  
    }

    inline Vec2 lerpv2(Vec2 a, Vec2 b, float t) {
        t = std::min(std::max(0.0f, t), 1.0f);
        return (1 - t) * a + b * t;  
    }

    inline Vec3 lerpv3(Vec3 a, Vec3 b, float t) {
        t = std::min(std::max(0.0f, t), 1.0f);
        return (1 - t) * a + b * t;  
    }

    inline constexpr float fmap_to_range(float input_begin, float input_end, float output_begin, float output_end, float t) {
        float slope = (output_end - output_begin) / (input_end - input_begin);
        return output_begin + slope * (t - input_begin);
    }

    // NOTE: SDL MUST hae been initialized prior to calling ANY function
    // in these namespaces
    namespace Input {
        namespace Keyboard {
            class Key {
            public:
                SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
                bool may_repeat = true;

                void set_repeat_delay(double delay) { m_repeat_delay.set_countdown_time(delay); }
                void set_repeat_interval(double interval) { m_repeat_interval.set_countdown_time(interval); }
            private:
                enum class KeyState {
                    Up,
                    Pressed,
                    Wait,
                    Repeat
                };
                KeyState m_keystate{0};

                Countdown m_repeat_delay{0.27, true};
                Countdown m_repeat_interval{0.025, true};

                friend class InputHandler;
                friend bool may_press(Key&, double);
                // may_press could be a method instead of a free function, but I feel it makes more sense
                // for it to be a free function
            };
            // returns true if a key is currently registering a press. returns false otherwise.
            // a return value of false might also indicate that SDL hasn't been initialized yet
            bool may_press(Key& k, double delta_time);
            // returns true if a key is being held down. returns false otherwise.
            // a return value of false might also indicate that SDL hasn't been initialized yet
            bool is_down(SDL_Scancode key);
            // returns true if a key is currently not being pressed. returns false otherwise.
            // a return value of false might also indicate that SDL hasn't been initialized yet
            bool is_up(SDL_Scancode key);
        }

        namespace Mouse {
            enum class MButtons {
                Left = SDL_BUTTON_LEFT,
                Middle = SDL_BUTTON_MIDDLE,
                Right = SDL_BUTTON_RIGHT,
                SideButton1 = SDL_BUTTON_X1,
                SideButton2 = SDL_BUTTON_X2,
            };

            // returns true if the left mouse button is currently registering a press. returns false otherwise.
            // a return value of false might also indicate that SDL hasn't been initialized yet
            bool left_button_down(void);
            // returns true if if the middle mouse button is currently registering a press. returns false otherwise.
            // a return value of false might also indicate that SDL hasn't been initialized yet
            bool middle_button_down(void);
            // returns true if if the right mouse button is currently registering a press. returns false otherwise.
            // a return value of false might also indicate that SDL hasn't been initialized yet
            bool right_button_down(void);
            // returns true if a mouse button is currently registering a press. returns false otherwise.
            // a return value of false might also indicate that SDL hasn't been initialized yet
            bool is_button_down(MButtons button);

            // returns the position of the mouse relative to the window
            Vec2 position();
            // returns the difference between mouse positions since the last frame.
            Vec2 delta();
        }
    }
}

template<>
struct std::formatter<TEngine::Vec2> {
    constexpr auto parse(std::format_parse_context& ctx) {
        if (*ctx.begin() != '}') {
            throw std::format_error("Vec2 formatter: no specifiers allowed.");
        }
        return ctx.begin();
    }

    auto format(TEngine::Vec2 v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({},{})", v.x, v.y);
    }
};

template<>
struct std::formatter<TEngine::Vec3> {
    constexpr auto parse(std::format_parse_context& ctx) {
        if (*ctx.begin() != '}') {
            throw std::format_error("Vec3 formatter: no specifiers allowed.");
        }
        return ctx.begin();
    }

    auto format(TEngine::Vec3 v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({},{},{})", v.x, v.y, v.z);
    }
};

template<>
struct std::formatter<TEngine::ColorHSB> {
    constexpr auto parse(std::format_parse_context& ctx) {
        if (*ctx.begin() != '}') {
            throw std::format_error("ColorHSB formatter: no specifiers allowed.");
        }
        return ctx.begin();
    }

    auto format(const TEngine::ColorHSB& c, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {} {} {}", c.hue, c.sat, c.bri, c.alpha);
    }
};

template<>
struct std::formatter<TEngine::Color> {
    bool __hex = false;
    bool __include_alpha = false;
    bool __capital_hex = false;
    constexpr auto parse(std::format_parse_context& ctx) {
        auto pos = ctx.begin();
        while (pos != ctx.end() && *pos != '}') {
            if (*pos == 'x' || *pos == 'X') {
                __hex = true;
                __capital_hex = *pos == 'X';
            } else if (*pos == 'a' || *pos == 'A') {
                __include_alpha = true;
            } else {
                throw std::format_error("Color formatter: only formatters allowed are x, X, a and A");
            }
            ++pos;
        }
        return pos;
    }

    auto format(TEngine::Color c, std::format_context& ctx) const {
        if (__hex) {
            uint32_t shift = __include_alpha ? 0u : 8u;
            uint32_t mask = __include_alpha ? ~0u : 0u;
            uint32_t v = (c.r << (24 - shift)) | (c.g << (16 - shift)) | (c.b << (8 - shift)) | (c.a & mask); 
            if (__capital_hex) {
                return std::format_to(ctx.out(), "#{:0<{}X}", v, __include_alpha ? 8 : 6);
            }
            return std::format_to(ctx.out(), "#{:0<{}x}", v, __include_alpha ? 8 : 6);
        }
        
        if (__include_alpha) {
            return std::format_to(ctx.out(), "{} {} {} {}", c.r, c.g, c.b, c.a);
        }
        return std::format_to(ctx.out(), "{} {} {}", c.r, c.g, c.b);
    }
};