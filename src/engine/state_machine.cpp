#include "state_machine.hpp"

namespace TEngine::SM {
    GenericStateMachine& GenericStateMachine::add_state(const std::string& id, std::unique_ptr<States::IState> state) {
        m_states.insert(std::make_pair(id, std::move(state)));
        return *this;
    }
    void GenericStateMachine::remove_state(const std::string& id) {
        m_states.erase(id);
    }

    void GenericStateMachine::clear(void) {
        m_states.clear();
        m_next.clear();
        m_curr = nullptr;
    }

    void GenericStateMachine::switch_to(const std::string& id) {
        // if no state is active, switch immediately
        if (!m_curr) {
            m_curr = m_states.at(id).get();
            m_curr->enter();
        } else {
            m_next = id;
        }
    }
    
    // void GenericStateMachine::update(double delta_t) {
    //     update_state();
    //     m_curr->update(delta_t);
    // }

    void GenericStateMachine::update_state(void) {
        if (!m_next.empty()) {
            // m_curr->reset();
            m_curr->exit();
            m_curr = m_states.at(m_next).get();
            m_curr->enter();
            m_next.clear();
        }
    }
}