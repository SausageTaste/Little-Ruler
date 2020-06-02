#include "d_widget_manager.h"


namespace dal {

    Widget2D* resolveNewFocus(Widget2D* const lastFocused, Widget2D* const newlyFocused) {
        if ( nullptr == newlyFocused ) {
            return lastFocused;
        }
        else if ( lastFocused == newlyFocused ) {
            return lastFocused;
        }
        else {  // New one has got focus.
            if ( nullptr != lastFocused ) {
                lastFocused->onFocusChange(false);
            }
            newlyFocused->onFocusChange(true);

            return newlyFocused;
        }
    }

}


namespace dal {

    auto InputDispatcher::dispatch(Widget2D** frontWidget, Widget2D** const end, const TouchEvent& e) -> std::pair<dal::InputDealtFlag, Widget2D*> {
        auto& state = this->getOrMakeTouchState(e.m_id, this->m_states);

        if ( nullptr != state.m_owner ) {
            const auto ctrlFlag = state.m_owner->onTouch(e);
            switch ( ctrlFlag ) {

            case dal::InputDealtFlag::consumed:
                state.m_owner = nullptr;
                return { dal::InputDealtFlag::consumed, nullptr };
            case dal::InputDealtFlag::owned:
                return { dal::InputDealtFlag::owned, state.m_owner };
            case dal::InputDealtFlag::ignored:
                state.m_owner = nullptr;
                break;  // Enters widgets loop below.
            default:
                assert(false);

            }
        }

        for ( ; end != frontWidget; ++frontWidget ) {
            Widget2D* w = *frontWidget;
            if ( !w->aabb().isInside(e.m_pos) ) {
                continue;
            }

            const auto ctrlFlag = w->onTouch(e);
            switch ( ctrlFlag ) {

            case dal::InputDealtFlag::consumed:
                return { dal::InputDealtFlag::consumed, w };
            case dal::InputDealtFlag::ignored:
                continue;
            case dal::InputDealtFlag::owned:
                state.m_owner = w;
                return { dal::InputDealtFlag::owned, w };
            default:
                assert(false);
            }
        }

        return { dal::InputDealtFlag::ignored, nullptr };
    }

    auto InputDispatcher::dispatch(Widget2D** frontWidget, Widget2D** const end, const KeyboardEvent& e, const KeyStatesRegistry& keyStates) -> dal::InputDealtFlag {
        for ( ; end != frontWidget; ++frontWidget ) {
            const auto flag = (*frontWidget)->onKeyInput(e, keyStates);
            if ( dal::InputDealtFlag::ignored != flag ) {
                return flag;
            }
        }

        return dal::InputDealtFlag::ignored;
    }

    void InputDispatcher::notifyWidgetRemoved(const dal::Widget2D* const w) {
        for ( auto& [id, state] : this->m_states ) {
            if ( w == state.m_owner ) {
                state.m_owner = nullptr;
                return;
            }
        }
    }

    // Private

    auto InputDispatcher::getOrMakeTouchState(const dal::touchID_t id, states_t& states) const -> TouchState& {
        auto found = states.find(id);
        if ( this->m_states.end() != found ) {
            return found->second;
        }
        else {
            return states.emplace(id, TouchState{}).first->second;
        }
    }

}
