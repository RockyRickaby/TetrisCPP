#pragma once

#include <functional>
#include <typeindex>
#include <cstdint>
#include <vector>
#include <utility>
#include <iostream>
#include <SDL3/SDL.h>

// event stuff taken from The Cherno's mini-series on application architecture
// https://github.com/TheCherno/Architecture/blob/main/Core/Source/Core/Event.h
#define EVENT_CLASS_TYPE(type) static EventType get_static_type() { return EventType::type; }\
								virtual EventType get_event_type() const override { return get_static_type(); }\
								virtual const char* get_name() const override { return #type; }

namespace TEngine {
    namespace Events {
        enum class EventType {
            None = 0,
            KeyPressed, KeyDown, KeyReleased,
            MousePressed, MouseDown, MouseReleased, MouseMoved,
        };

        // TODO - maybe reduce the object-orientation of this
        class IEvent {
        public:
            bool handled = false;

            virtual ~IEvent() = default;

            virtual EventType get_event_type() const = 0;
            virtual const char* get_name() const = 0;
        };

        class KeyPressedEvent : public IEvent {
        public:
            KeyPressedEvent(SDL_Scancode scancode) : m_scancode{scancode} {}
            SDL_Scancode get_scancde() const { return m_scancode; }
            
            EVENT_CLASS_TYPE(KeyPressed)
        private:
            SDL_Scancode m_scancode;
        };

        class KeyRepeatEvent : public IEvent {
        public:
            KeyRepeatEvent(SDL_Scancode scancode) : m_scancode{scancode} {}
            SDL_Scancode get_scancde() const { return m_scancode; }
            
            EVENT_CLASS_TYPE(KeyDown)
        private:
            SDL_Scancode m_scancode;
        };

        class KeyReleasedEvent : public IEvent {
        public:
            KeyReleasedEvent(SDL_Scancode scancode) : m_scancode{scancode} {}
            SDL_Scancode get_scancde() const { return m_scancode; }
            
            EVENT_CLASS_TYPE(KeyReleased)
        private:
            SDL_Scancode m_scancode;
        };

        // handful abstract class for implementing event methods
        class EventListener {
        public:
            virtual ~EventListener() = default;
            virtual void event(IEvent&) {};
        protected:
            // keyboard events
            virtual bool OnKeyPressed(KeyPressedEvent&) { return false; }
            virtual bool OnKeyReleased(KeyReleasedEvent&) { return false; }
            virtual bool OnKeyRepeat(KeyRepeatEvent&) { return false; }

            // mouse events
            // TODO - implement these events
            virtual bool OnMousePressed(IEvent&) { return false; }
            virtual bool OnMouseReleased(IEvent&) { return false; }
            virtual bool OnMouseDown(IEvent&) { return false; }
        };

        class EventDispatcher final {
        private:
            template<typename T>
            using EventCallback = std::function<bool(T&)>;
        public:
            EventDispatcher(IEvent& event) : m_event{event} {}
            template<typename EventType> requires(std::is_base_of_v<IEvent, EventType>)
            bool dispatch(EventCallback<EventType> f) {
                if (m_event.get_event_type() == EventType::get_static_type() && !m_event.handled) {
                    m_event.handled = f(*dynamic_cast<EventType*>(&m_event));
                    return true;
                }
                return false;
            }
        private:
            IEvent& m_event;
        };

        class ICustomEvent {
        public:
            virtual ~ICustomEvent() = default;

            virtual const char* get_name() const = 0;
        };

        // NOTE - not a very good implementation... beware
        class CustomEventsDispatcher final {
        private:
            using EventCallback = std::function<bool(const ICustomEvent*)>;
            using ListenerHandle = std::uint64_t;
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