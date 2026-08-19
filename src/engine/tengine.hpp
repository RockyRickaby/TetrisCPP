#pragma once

#include <memory>
#include <cstdint>
#include <iostream>
#include <SDL3/SDL.h>

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
        double get_countdown_time(void);
        void reset(void);
    private:
        double m_time_counter;
        double m_time_delta;
    };

    // this is a very small class. you can just copy it around with no issues.
    // use the [] operator to get the state of a key (indexed by an SDL_Scancode).
    // it's better to just use InputHandler if all that's needed is to
    // check the input during update, as it allows some customization as to
    // how often and how quickly keys may fire their input when held down.
    // Not a singleton. May be instantiated as many times as desired. It will make no difference.
    // No memory is managed by it. The underlying keyboard pointer is managed by SDL.
    // NOTE: SDL MUST BE INITIALIZED for any meaningful results to be returned
    // (otherwise, every check will return false)
    class KeyboardState {
    public:
        KeyboardState() { init_keyboard(); }
        bool operator[](SDL_Scancode scancode) { return init_keyboard() ? m_keyboard[scancode] : false; }
        bool down(SDL_Scancode scancode) { return init_keyboard() ? (*this)[scancode] : false; }
        bool up(SDL_Scancode scancode) { return init_keyboard() ? !(*this)[scancode] : false; }
    private:
        static const bool* m_keyboard;
        // returns true if the initialization was successful.
        // may be called as many times as desired, as it will be initialized only once!
        static bool init_keyboard();
    };

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
        TEngine::Time m_timer{};
        KeyState m_keystate{0};

        TEngine::Countdown m_repeat_delay{0.27, true};
        TEngine::Countdown m_repeat_interval{0.025, true};

        friend class InputHandler;
    };

    // This class uses 1 byte of memory. Instantiate it as many times as you wish.
    // This class does not manage any memory at all
    class InputHandler {
    public:
        // a small state machine is implemented to handle how quickly
        // the keys repeat when held down and how long it takes to
        // fire the key repeatedly when first held down.
        // you can mess around with this behavior by changing the time of timers in the Key object
        bool may_press(Key &key);
    private:
        KeyboardState m_kb;
        bool may_press_state(Key &key, KeyboardState kb);
    };

    // NOTE: already available in SDL as SDL_Color
    struct Color {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 0;

        bool operator==(const Color other) const;
        bool operator!=(const Color other) const;

        friend std::ostream& operator<<(std::ostream& output, const Color& v);
    };
 
    struct Vec2 {
        float x = 0;
        float y = 0;

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
}