#pragma once

#include "tengine.hpp"
#include <cstdint>
#include <functional>
#include <typeindex>
#include <variant>
#include <vector>
#include <utility>
#include <SDL3/SDL.h>

// event stuff taken and adapted from The Cherno's mini-series on application architecture
// https://github.com/TheCherno/Architecture/blob/main/Core/Source/Core/Event.h
#define EVENT_CLASS_TYPE(type) static EventType get_static_type() { return EventType::type; }\
								virtual EventType get_event_type() const override { return get_static_type(); }\
								virtual const char* get_name() const override { return #type; }

namespace TEngine {
    namespace Events {
        enum class EventType {
            None = 0,
            KeyPressed, KeyRepeat, KeyReleased,
            MousePressed, MouseReleased, MouseMoved, MouseWheel,
            WindowResized,
            TextInput, TextEditing
        };

        // All main events have to implement this interface to work
        // with the main event dispatcher
        class IEvent {
        public:
            bool handled = false;

            virtual ~IEvent() = default;

            virtual EventType get_event_type() const = 0;
            virtual const char* get_name() const = 0;
        };

        class KeyPressedEvent : public IEvent {
        public:
            KeyPressedEvent(SDL_Scancode scancode) : scancode{scancode} {}

            const SDL_Scancode scancode;
            EVENT_CLASS_TYPE(KeyPressed)
        };

        class KeyRepeatEvent : public IEvent {
        public:
            KeyRepeatEvent(SDL_Scancode scancode) : scancode{scancode} {}

            const SDL_Scancode scancode;
            EVENT_CLASS_TYPE(KeyRepeat)
        };

        class KeyReleasedEvent : public IEvent {
        public:
            KeyReleasedEvent(SDL_Scancode scancode) : scancode{scancode} {}
            
            const SDL_Scancode scancode;
            EVENT_CLASS_TYPE(KeyReleased)
        };

        class MousePressedEvent : public IEvent {
        public:
            MousePressedEvent(int mouse_button, float x, float y, int clicks) :
                clicks{clicks},
                m_button{mouse_button},
                m_x{x},
                m_y{y}
            {}

            bool right_button_pressed() const { return m_button == SDL_BUTTON_RIGHT; }
            bool left_button_pressed() const { return m_button == SDL_BUTTON_LEFT; }
            Vec2 get_position() const { return { m_x, m_y }; }
            
            const int clicks;
            EVENT_CLASS_TYPE(MousePressed)
        private:
            int m_button;
            float m_x;
            float m_y;
        };

        class MouseReleasedEvent : public IEvent {
        public:
            MouseReleasedEvent(int mouse_button, float x, float y) :
                m_button{mouse_button},
                m_x{x},
                m_y{y}
            {}

            bool right_button_released() const { return m_button == SDL_BUTTON_RIGHT; }
            bool left_button_released() const { return m_button == SDL_BUTTON_LEFT; }
            Vec2 get_position() const { return { m_x, m_y }; }

            EVENT_CLASS_TYPE(MouseReleased)
        private:
            int m_button;
            float m_x;
            float m_y;
        };

        class MouseMovedEvent : public IEvent {
        public:
            MouseMovedEvent(float x, float y, float dx, float dy) :
                m_x{x},
                m_y{y},
                m_dx{dx},
                m_dy{dy}
            {}

            Vec2 get_direction() const { return { m_dx, m_dy }; }
            Vec2 get_position() const { return { m_x, m_y }; }

            EVENT_CLASS_TYPE(MouseMoved)
        private:
            float m_x;
            float m_y;
            float m_dx;
            float m_dy;
        };

        class MouseWheelEvent : public IEvent {
        public:
            MouseWheelEvent(float scroll_dx, float scroll_dy, float pos_x, float pos_y, bool flipped) :
                flipped{flipped},
                m_x{pos_x},
                m_y{pos_y},
                m_dx{scroll_dx},
                m_dy{scroll_dy}
            {}
            
            Vec2 get_position() const { return { m_x, m_y }; }
            Vec2 get_scroll_dir() const { return { m_dx, m_dy}; }
            
            const bool flipped;
            EVENT_CLASS_TYPE(MouseWheel)
        private:
            float m_x;
            float m_y;
            float m_dx;
            float m_dy;
        };

        class WindowResizedEvent : public IEvent {
        public:
            WindowResizedEvent(int new_width, int new_height) :
                width{new_width},
                height{new_height}
            {}

            Vec2 get_size() const { return {static_cast<float>(width), static_cast<float>(height)}; }

            const int width;
            const int height;
            EVENT_CLASS_TYPE(WindowResized)
        };

        class TextInputEvent : public IEvent {
        public:
            TextInputEvent(const char* text) : text{text} {}

            const std::string text;

            EVENT_CLASS_TYPE(TextInput)
        };

        class TextEditingEvent : public IEvent {
        public:
            TextEditingEvent(const char* text, std::int32_t start, std::int32_t length) :
                text{text},
                start{start},
                length{length}
            {}

            const std::string text;
            const std::int32_t start;
            const std::int32_t length;

            EVENT_CLASS_TYPE(TextEditing)
        };

        // not really necessary unless polymorphic event listeners are neeed.
        // as long as you have a function that takes an event, you can use the event dispatcher
        class EventListener {
        public:
            virtual ~EventListener() = default;
            virtual void event(IEvent&) = 0;
        };

        // convenience function. it constructs the Event on the stack
        // and forwards it to the receiver R.
        // this function is not mandatory to use in case you want to forward events
        template<typename Event, typename R, typename ...Args>
        requires(std::is_base_of_v<IEvent, Event> && requires(R receiver, Event evemt) {
            { receiver.event(evemt) } -> std::same_as<void>;
        })
        void forward_event(R& receiver, Args&& ...args) {
            auto ev = Event{std::forward<Args>(args)...};
            receiver.event(ev);
        }
        
        // handful abstract class for implementing event methods
        class EventListenerFunctions {
        public:
            virtual ~EventListenerFunctions() = default;
            // keyboard events
            virtual bool OnKeyPressed(KeyPressedEvent&) { return false; }
            virtual bool OnKeyReleased(KeyReleasedEvent&) { return false; }
            virtual bool OnKeyRepeat(KeyRepeatEvent&) { return false; }

            // mouse events
            virtual bool OnMousePressed(MousePressedEvent&) { return false; }
            virtual bool OnMouseReleased(MouseReleasedEvent&) { return false; }
            virtual bool OnMouseMoved(MouseMovedEvent&) { return false; }
            virtual bool OnMouseScroll(MouseWheelEvent&) { return false; };

            // window events
            virtual bool OnWindowResize(WindowResizedEvent&) { return false; };
        };

        template<typename FN, typename EventType>
        concept DispatcherCallback = requires(FN fn, EventType ev) {
            { fn(ev) } -> std::same_as<bool>;
        } && std::is_base_of_v<IEvent, EventType>;

        // This is an alternative to the dispatch_event(IEvent&) function
        class EventDispatcher final {
        public:
            EventDispatcher(IEvent& event) : m_event{event} {}
            // This function should be called for all events that you may want to respond to.
            // Example:
            // dispatch_event<MouseEvent>(event);
            // dispatch_event<KeyEvent>(event);
            template<typename EventType, DispatcherCallback<EventType> EventCallback> requires(std::is_base_of_v<IEvent, EventType>)
            bool dispatch(EventCallback f) {
                if (m_event.get_event_type() == EventType::get_static_type() && !m_event.handled) {
                    m_event.handled = f(*dynamic_cast<EventType*>(&m_event));
                    return true;
                }
                return false;
            }
        private:
            IEvent& m_event;
        };

        // This function should be called for all events that you may want to respond to.
        // This is an alternative to the EventDispatcher class
        // Example:
        // dispatch_event<MouseEvent>(event);
        // dispatch_event<KeyEvent>(event);
        template<typename EventType, DispatcherCallback<EventType> EventCallback> requires(std::is_base_of_v<IEvent, EventType>)
        bool dispatch_event(IEvent& event, EventCallback f) {
            if (event.get_event_type() == EventType::get_static_type() && !event.handled) {
                event.handled = f(*dynamic_cast<EventType*>(&event));
                return true;
            }
            return false;
        }

        class ICustomEvent {
        public:
            virtual ~ICustomEvent() = default;

            virtual const char* get_name() const = 0;
        };

        // NOTE - not a very good implementation... beware
        class CustomEventsDispatcher final {
        private:
            using EventCallback = std::function<bool(const ICustomEvent*)>;
        public:
            CustomEventsDispatcher() : queue{}, m_event_callbacks{} {}

            // inserts or overwrites the registered callback for the given EventType
            template<typename EventType> requires(std::is_base_of_v<ICustomEvent, EventType>)
            void listen(EventCallback f) {
                m_event_callbacks.insert_or_assign(typeid(EventType), f);
            }

            // removes the callback associated with EventType
            template<typename EventType> requires(std::is_base_of_v<ICustomEvent, EventType>)
            bool remove(void) {
                const std::type_index& event_key = typeid(EventType);
                if (m_event_callbacks.contains(event_key)) {
                    m_event_callbacks.erase(event_key);
                    return true;
                }
                return false;
            }

            // dispatches the custom event to the registered callback.
            // the event itself may contain some or no data if necessary.
            // returns true if the dispatching was successful.
            // returns false if no callback is available for the EventType.
            template<typename EventType> requires(std::is_base_of_v<ICustomEvent, EventType>)
            bool dispatch(EventType& event) {
                const std::type_index& event_key = typeid(EventType);
                if (m_event_callbacks.contains(event_key)) {
                    m_event_callbacks.at(typeid(EventType))(&event);
                    return true;
                }
                return false;
            }

            // enqueues an event to be dispatched later by calling dispatch_enqueued().
            // it does not affect the behavior of the dispatch() function.
            // returns true if there is a callback available and if the event was successfully enqueued.
            // returns false if there's no callback available for the EventType.
            template<typename EventType> requires(std::is_base_of_v<ICustomEvent, EventType>)
            bool enqueue(EventType&& event) {
                if (m_event_callbacks.contains(typeid(EventType))) {
                    queue.emplace_back([ev = std::forward<EventType>(event)](CustomEventsDispatcher& ed){
                        ed.dispatch(ev);
                    });
                    return true;
                }
                return false;
            }

            // dispatches all enqueued events (in FIFO order) and clears the queue.
            void dispatch_enqueued(void) {
                for (const auto& cb : queue) {
                    cb(*this);
                }
                queue.clear();
            }
        private:
            std::vector<std::function<void(CustomEventsDispatcher&)>> queue;
            std::unordered_map<std::type_index, EventCallback> m_event_callbacks;
        };
    }
}