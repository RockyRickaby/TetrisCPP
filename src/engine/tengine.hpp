#pragma once

#include <SDL3/SDL_oldnames.h>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <SDL3/SDL.h>
#include <random>
#include <type_traits>

// TODO - implement little 3D software renderer as seen in mista a-zozin's video (https://github.com/tsoding/formula)
namespace TEngine {
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

    // Countdown timer. Time is assumed to be in seconds
    class Countdown {
    public:
        // easir to just make this public than have a setter
        bool autoreset;

        Countdown(double count, bool autoreset = false);
        bool done(double delta_t);
        void set_countdown_time(double time);
        double get_countdown_time(void) { return m_time_delta; }
        double get_time_left(void) { return m_time_counter; }
        void reset(void);
    private:
        double m_time_counter;
        double m_time_delta;
    };

    class Random {
    public:
        Random() = default;
        Random(std::mt19937::result_type seed) : eng{seed} {};

        int draw_int(int min, int max) { return std::uniform_int_distribution<int>{min, max}(eng); }
        float draw_float(float min, float max) { return std::uniform_real_distribution<float>{min, max}(eng); }
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

    using ColorHSV = ColorHSB;
 
    struct Vec2 {
        float x = 0;
        float y = 0;

        Vec2 normalize() const {
            float len = length();
            return {
                x / len,
                y / len
            };
        }

        float length() const { return std::sqrtf(x * x + y * y); }
        float dot(const Vec2 other) const { return (x * other.x) + (y * other.y); }
        float cross2D(const Vec2 other) const { return (x * other.y) - (y * other.x); }

        Vec2 operator+(const Vec2 other) const;
        Vec2 operator-(const Vec2 other) const;
        Vec2 operator*(float scalar) const;
        Vec2& operator+=(const Vec2 other);
        Vec2& operator-=(const Vec2 other);
        Vec2& operator*=(float scalar);
        bool operator==(const Vec2 other) const;
        bool operator!=(const Vec2 other) const;

        friend std::ostream& operator<<(std::ostream& output, const Vec2& v);
    };

    struct Vec3 {
        float x = 0;
        float y = 0;
        float z = 0;

        Vec3 normalize() const {
            float len = length();
            return {
                x / len,
                y / len,
                z / len
            };
        }

        float length() const { return std::sqrtf(x * x + y * y + z * z); }
        float dot(const Vec3& other) const {
            return
                (x * other.x) +
                (y * other.y) +
                (z * other.z);
        }
        Vec3 cross(const Vec3& other) const {
            return {
                (y * other.z) - (z * other.y),
                (z * other.x) - (x * other.z),
                (x * other.y) - (y * other.x)
            };
        }

        Vec3 operator+(const Vec3 other) const;
        Vec3 operator-(const Vec3 other) const;
        Vec3 operator*(float scalar) const;
        Vec3& operator+=(const Vec3 other);
        Vec3& operator-=(const Vec3 other);
        Vec3& operator*=(float scalar);
        bool operator==(const Vec3 other) const;
        bool operator!=(const Vec3 other) const;

        friend std::ostream& operator<<(std::ostream& output, const Vec3& v);
    };

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
                Time m_timer{};
                KeyState m_keystate{0};

                Countdown m_repeat_delay{0.27, true};
                Countdown m_repeat_interval{0.025, true};

                friend class InputHandler;
                friend bool may_press(Key&);
                // may_press could be a method instead of a free function, but I feel it makes more sense
                // for it to be a free function
            };
            // returns true if a key is currently registering a press. returns false otherwise.
            // a return value of false might also indicate that SDL hasn't been initialized yet
            bool may_press(Key& k);
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