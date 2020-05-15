#pragma once

#include <d_input_data.h>

#include "d_widget.h"


namespace dal {

    // Returns either last focused or newly focused.
    // Either or both or none of two can be nullptr.
    Widget2D* resolveNewFocus(Widget2D* const lastFocused, Widget2D* const newlyFocused);


    class InputDispatcher {

    private:
        struct TouchState {
            Widget2D* m_owner = nullptr;
        };

    private:
        using states_t = std::unordered_map<touchID_t, TouchState>;
        states_t m_states;

    public:
        // Returns a pointer to a widget that has gotten focus on.
        // Nullptr if nothing has gotten.
        auto dispatch(Widget2D** begin, Widget2D** const end, const TouchEvent& e)->std::pair<InputDealtFlag, Widget2D*>;
        auto dispatch(Widget2D** begin, Widget2D** const end, const KeyboardEvent& e, const KeyStatesRegistry& keyStates)->InputDealtFlag;

        void notifyWidgetRemoved(const dal::Widget2D* const w);

    private:
        auto getOrMakeTouchState(const dal::touchID_t id, states_t& states) const->TouchState&;

    };

}
