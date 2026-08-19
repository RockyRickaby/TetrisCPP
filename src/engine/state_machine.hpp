#pragma once

#include <type_traits>
#include <unordered_map>
#include <memory>
#include <typeindex>

#include "events.hpp"

// TODO - reduce indirection a little
namespace TEngine::StateMachine {
    // State base class to be used for classes (states) that may be used by a StateMachine
    class State : public Events::EventListener {
    public:
        virtual void enter(void) {}
        // the double argument is for the delta_time
        virtual void update(double) {}
        virtual void draw(void) {}
        virtual void reset(void) {}
        virtual void exit(void) {}

        virtual ~State() = default;
    };

    class StateMachine : public Events::EventListener {
    public:
        virtual void update(double delta) = 0;
        virtual void draw(void) = 0;
        
        virtual ~StateMachine() = default;
    };

    // non-virtual destructor.
    // this class will handle the lifetime of the states by itself
    template<typename Key = std::type_index, typename __Hash = std::hash<Key>>
    class GenericStateMachine : public StateMachine {
    public:
        // must call switch_to() before handling anything
        GenericStateMachine() : m_curr(nullptr), m_next{}, m_states{}, m_switching{false} {}
        GenericStateMachine(Key init_state_id, std::unordered_map<Key, std::unique_ptr<State, __Hash>>&& states) :
            m_curr(nullptr),
            m_next{},
            m_states{std::move(states)},
            m_switching{false}
        {
            m_curr = m_states.at(init_state_id).get();
        }

        template<typename T, typename... Args> requires(std::is_base_of_v<State, T>)
        GenericStateMachine& add_state(const Key& id, Args&&... args) {
            m_states.emplace(id, std::make_unique<T>(std::forward<Args>(args)...));
            return *this;
        }
        void remove_state(const Key& id) {
            m_states.erase(id);
        }
        void clear(void) {
            m_states.clear();
            m_next.clear();
            m_curr = nullptr;
        }
        void switch_to(const Key& id) {
            // if no state is active, switch immediately
            if (!m_curr) {
                m_curr = m_states.at(id).get();
                m_curr->enter();
            } else {
                m_switching = true;
                m_next = id;
            }
        }
        
        void update(double delta_t) override { update_state(); m_curr->update(delta_t); }
        void event(Events::IEvent& event) override { m_curr->event(event); }
        void draw(void) override {
            m_curr->draw();
        }
    private:
        State *m_curr;
        Key m_next;
        std::unordered_map<Key, std::unique_ptr<State>, __Hash> m_states;
        bool m_switching;

        void update_state(void) {
            if (m_switching) {
                m_curr->exit();
                m_curr = m_states.at(m_next).get();
                m_curr->enter();
                m_switching = false;
            }
        }
    };
}