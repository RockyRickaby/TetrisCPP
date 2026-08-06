#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace TEngine::SM {
    // change this in case different handlers are needed for each
    namespace States {
        // interface for the states to use in the StateMachine
        class IState {
        public:
            virtual void enter(void) = 0;
            virtual void update(double delta_t) = 0;
            virtual void handle_input(void) = 0;
            virtual void reset(void) = 0;
            virtual void exit(void) = 0;

            virtual ~IState() = default;
        };

        // some states might require drawing (the ones in tetris_game_sm.hpp certainly do)
        class IDrawableState : public IState {
        public:
            virtual void draw(void) = 0;
            virtual ~IDrawableState() = default;
        };

        // may be cast to IState* as well
        class EmptyState final : public IDrawableState {
        public:
            void enter(void) override {}
            void update(double delta_t) override {}
            void handle_input(void) override {}
            void reset(void) override {};
            void exit(void) override {}
            void draw(void) override {}
        };
    }

    class IStateMachine {

    };

    // TODO - move some of these to the /tetris/ folder
    // non-virtual destructor
    class GenericStateMachine {
    public:
        // must call switch_to() before handling anything
        GenericStateMachine() : m_curr(nullptr), m_next{""}, m_states{} {}
        GenericStateMachine(std::string init_state_id, std::unordered_map<std::string, std::unique_ptr<States::IState>>&& states) : m_curr(nullptr), m_next{""}, m_states{std::move(states)} {
            m_curr = m_states.at(init_state_id).get();
        }

        // TODO - consider option of using shared_ptr () or raw pointers
        GenericStateMachine& add_state(const std::string& id, std::unique_ptr<States::IState> state);
        void remove_state(const std::string& id);
        void clear(void);
        void switch_to(const std::string& id);
        
        void update(double delta_t) { update_state(); m_curr->update(delta_t); }
        void handle_input(void) { m_curr->handle_input(); }
        // TODO - mm... consider whether to put rendering logic somewhere else
        void draw(void) {
            States::IDrawableState *drawable = dynamic_cast<States::IDrawableState*>(m_curr);
            if (drawable) {
                drawable->draw();
            }
        }
    private:
        States::IState *m_curr;
        std::string m_next;
        std::unordered_map<std::string, std::unique_ptr<States::IState>> m_states;

        void update_state(void);
    };
}