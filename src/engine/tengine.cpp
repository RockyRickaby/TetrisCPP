#include <algorithm>
#include <format>
#include "tengine.hpp"

namespace TEngine {
    // convenience if we don't want to create a new Random instance every time
    // we need a random number 
    static Random rnd;
    // const bool* KeyboardState::m_keyboard = nullptr;

    int random_int(int min, int max) {
        return rnd.draw_int(min, max);
    }

    float random_float(float min, float max) {
        return rnd.draw_float(min, max);
    }

    Time::Time() :
        m_freq{SDL_GetPerformanceFrequency()},
        time_last{SDL_GetPerformanceCounter()}
    {}

    double Time::delta_time(void) {
        Uint64 time_now = SDL_GetPerformanceCounter();
        double delta_time = (static_cast<double>(time_now - time_last) / static_cast<double>(m_freq));
        time_last = time_now;
        return delta_time;
    }

    Countdown::Countdown(double count, bool autoreset) :
        autoreset{autoreset},
        m_time_counter{count},
        m_time_delta{count}
    {}

    bool Countdown::done(double delta_t) {
        m_time_counter -= delta_t;
        if (m_time_counter <= 0) {
            if (autoreset) {
                reset();
            } else {
                m_time_counter = 0; // unlikely to happen, but prevent the value from getting tooooooo small
            }
            return true;
        }
        return false;
    }

    void Countdown::set_countdown_time(double time) {
        m_time_delta = time;
        reset();
    }

    void Countdown::reset(void) {
        m_time_counter = m_time_delta;
    }

    namespace Input {
        namespace Keyboard {
            static const bool* keyboard = nullptr;

            static bool init_keyboard() {
                if (keyboard == nullptr && SDL_WasInit(SDL_INIT_VIDEO)) {
                    keyboard = SDL_GetKeyboardState(nullptr);
                }
                return keyboard != nullptr;
            }

            bool is_down(SDL_Scancode key) { return init_keyboard() ? keyboard[key] : false; }
            bool is_up(SDL_Scancode key) { return init_keyboard() ? !keyboard[key] : false; }

            bool may_press(Key& key) {
                if (!init_keyboard()) {
                    return false;
                }
                switch (key.m_keystate) {
                    case Key::KeyState::Up: {
                        if (is_down(key.scancode)) {
                            key.m_keystate = Key::KeyState::Pressed;
                            return true;
                        }
                    }; break;
                    case Key::KeyState::Pressed: {
                        if (is_down(key.scancode)) {
                            if (!key.may_repeat) {
                                return false;
                            }
                            if (key.m_repeat_delay.get_countdown_time() <= 0) {
                                key.m_keystate = Key::KeyState::Repeat;
                                return false;
                            }
                            key.m_keystate = Key::KeyState::Wait;
                            key.m_repeat_delay.done(key.m_timer.delta_time());
                            // if delay == 0, just jump to repeat state
                        } else {
                            key.m_keystate = Key::KeyState::Up;
                            key.m_repeat_delay.reset();
                            key.m_repeat_interval.reset();
                        }
                        return false;
                    }; break;
                    case Key::KeyState::Wait: {
                        if (is_down(key.scancode)) {
                            if (!key.m_repeat_delay.done(key.m_timer.delta_time())) {
                                return false;
                            }
                            key.m_keystate = Key::KeyState::Repeat;
                            return true;
                        } else {
                            key.m_keystate = Key::KeyState::Up;
                            key.m_repeat_delay.reset();
                            key.m_repeat_interval.reset();
                            return false;
                        }
                    }; break;
                    case Key::KeyState::Repeat: {
                        if (is_up(key.scancode)) {
                            key.m_keystate = Key::KeyState::Up;
                            key.m_repeat_delay.reset();
                            key.m_repeat_interval.reset();
                            return false;
                        } else if (!key.m_repeat_interval.done(key.m_timer.delta_time())) {
                            return false;
                        } else {
                            return true;
                        }
                    }; break;
                    default:
                        return false;
                }
                return false;
            }
        }

        namespace Mouse {

            static bool init_mouse() {
                return SDL_WasInit(SDL_INIT_VIDEO);
            }
            bool left_button_down(void) { return is_button_down(MButtons::Left); }
            bool middle_button_down(void) { return is_button_down(MButtons::Middle); }
            bool right_button_down(void) { return is_button_down(MButtons::Right); }

            bool is_button_down(MButtons button) {
                if (!init_mouse()) {
                    return false;
                }
                SDL_MouseButtonFlags buttons = SDL_GetMouseState(nullptr, nullptr);
                return buttons & SDL_BUTTON_MASK(static_cast<int>(button));
            }

            Vec2 position() {
                if (!init_mouse()) {
                    return {};
                }
                float x, y;
                [[maybe_unused]] SDL_MouseButtonFlags v = SDL_GetMouseState(&x, &y);
                return {x, y};
            }

            Vec2 delta() {
                if (!init_mouse()) {
                    return {};
                }
                float x, y;
                [[maybe_unused]] SDL_MouseButtonFlags v = SDL_GetRelativeMouseState(&x, &y);
                return {x, y};
            }
        }
    }

    bool Color::operator==(const Color other) const {
        return
            r == other.r &&
            g == other.g &&
            b == other.b &&
            a == other.a;
    }

    bool Color::operator!=(const Color other) const {
        return
            r != other.r ||
            g != other.g ||
            b != other.b ||
            a != other.a;   
    }

    bool ColorHSB::operator==(const ColorHSB& other) const {
        return 
            hue == other.hue &&
            sat == other.sat &&
            bri == other.bri &&
            alpha == other.alpha;
    }

    bool ColorHSB::operator!=(const ColorHSB& other) const {
        return 
            hue != other.hue ||
            sat != other.sat ||
            bri != other.bri ||
            alpha != other.alpha;
    }

    Vec2 Vec2::operator+(const Vec2 other) const {
        float x1 = this->x + other.x;
        float y1 = this->y + other.y;
        return Vec2{x1, y1};
    }

    Vec2 Vec2::operator*(float scalar) const {
        float x1 = this->x * scalar;
        float y1 = this->y * scalar;
        return Vec2{x1, y1};
    }

    Vec2& Vec2::operator+=(const Vec2 other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vec2 Vec2::operator-(const Vec2 other) const {
        float x1 = this->x - other.x;
        float y1 = this->y - other.y;
        return Vec2{x1, y1};
    }

    Vec2& Vec2::operator-=(const Vec2 other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vec2& Vec2::operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    bool Vec2::operator==(const Vec2 other) const {
        return this->x == other.x
            && this->y == other.y;
    }

    bool Vec2::operator!=(const Vec2 other) const {
        // return this->x != other.x || this->y != other.y;
        return !(*this == other);
    }

    Vec3 Vec3::operator+(const Vec3 other) const {
        float x1 = this->x + other.x;
        float y1 = this->y + other.y;
        float z1 = this->z + other.z;
        return Vec3{x1, y1, z1};
    }

    Vec3 Vec3::operator*(float scalar) const {
        float x1 = this->x * scalar;
        float y1 = this->y * scalar;
        float z1 = this->z * scalar;
        return Vec3{x1, y1, z1};
    }

    Vec3& Vec3::operator+=(const Vec3 other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3 Vec3::operator-(const Vec3 other) const {
        float x1 = this->x - other.x;
        float y1 = this->y - other.y;
        float z1 = this->z - other.z;
        return Vec3{x1, y1, z1};
    }

    Vec3& Vec3::operator-=(const Vec3 other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vec3& Vec3::operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    bool Vec3::operator==(const Vec3 other) const {
        return this->x == other.x
            && this->y == other.y
            && this->z == other.z;
    }

    bool Vec3::operator!=(const Vec3 other) const {
        return !(*this == other) ;
    }

    std::ostream& operator<<(std::ostream& output, const Color& v) {
        output << std::format("RGB: {} {} {} {}", v.r, v.g, v.b, v.a);
        return output;
    }

    std::ostream& operator<<(std::ostream& output, const ColorHSB& v) {
        output << std::format("HSB: {} {} {} {}", v.hue, v.sat, v.bri, v.alpha);
        return output;
    }

    std::ostream& operator<<(std::ostream& output, const Vec2& v) {
        output << std::format("({},{})", v.x, v.y);
        return output;
    }

    std::ostream& operator<<(std::ostream& output, const Vec3& v) {
        output << std::format("({},{},{})", v.x, v.y, v.z);
        return output;
    }

    void ColorHSB::increment_hue(float hue_inc) {
        hue_inc = std::clamp(hue_inc, 0.0f, 360.0f);
        hue += hue_inc;
        if (hue < 0) { hue += 360; }
        else if (hue >= 360) { hue -= 360; }
    }

    void ColorHSB::increment_sat(float sat_inc) {
        sat_inc = std::clamp(sat_inc, 0.0f, 1.0f);
        sat += sat_inc;
        sat = std::clamp(sat, 0.0f, 1.0f);
    }

    void ColorHSB::increment_bri(float bri_inc) {
        bri_inc = std::clamp(bri_inc, 0.0f, 1.0f);
        bri += bri_inc;
        bri = std::clamp(bri, 0.0f, 1.0f);
    }

    void ColorHSB::increment_alpha(float alpha_inc) {
        alpha_inc = std::clamp(alpha_inc, 0.0f, 1.0f);
        alpha += alpha_inc;
        alpha = std::clamp(alpha, 0.0f, 1.0f);
    }
}