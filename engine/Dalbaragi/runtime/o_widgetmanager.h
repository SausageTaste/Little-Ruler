#pragma once

#include <deque>
#include <vector>
#include <unordered_map>

#include "o_widgetbase.h"


namespace dal {

    // Returns either last focused or newly focused.
    // Either or both or none of two can be nullptr.
    Widget2* resolveNewFocus(Widget2* const lastFocused, Widget2* const newlyFocused);


    class WidgetStorage {

    private:
        // The last widget is on the top.
        std::vector<Widget2*> m_container;
        bool m_hasBackWidget = false, m_focusOnBG = false;

    public:
        void add(Widget2* const w) {
            this->m_container.push_back(w);
        }
        void addBack(Widget2* const w);
        bool remove(Widget2* const w) {
            const auto end = this->m_container.end();
            const auto found = std::find(this->m_container.begin(), this->m_container.end(), w);
            if ( end != found ) {
                this->m_container.erase(found);
                return true;
            }
            else {
                return false;
            }
        }

        void setFocusOn(Widget2* const w);

        auto begin(void) const {
            return this->m_container.begin();
        }
        auto end(void) const {
            return this->m_container.end();
        }

        auto getIterBack(void) const {
            return this->m_container.begin();
        }
        auto getEndIterBack(void) const {
            return this->m_container.end();
        }
        auto getIterFront(void) const {
            return this->m_container.rbegin();
        }
        auto getEndIterFront(void) const {
            return this->m_container.rend();
        }

    };


    class WidgetInputDispatcher {

    private:
        struct TouchState {
            dal::Widget2* m_owner = nullptr;
        };

    private:
        using states_t = std::unordered_map<dal::touchID_t, TouchState>;
        states_t m_states;

    public:
        // Returns a pointer to a widget that has gotten focus on.
        // Nullptr if nothing has gotten.
        template <typename _Iter>
        std::pair<dal::InputCtrlFlag, Widget2*> dispatch(_Iter frontWidget, const _Iter end, const TouchEvent& e) {
            auto& state = this->getOrMakeTouchState(e.m_id, this->m_states);

            if ( nullptr != state.m_owner ) {
                const auto ctrlFlag = state.m_owner->onTouch(e);
                switch ( ctrlFlag ) {

                case dal::InputCtrlFlag::consumed:
                    state.m_owner = nullptr;
                    return { dal::InputCtrlFlag::consumed, nullptr };
                case dal::InputCtrlFlag::owned:
                    return { dal::InputCtrlFlag::owned, state.m_owner };
                case dal::InputCtrlFlag::ignored:
                    state.m_owner = nullptr;
                    break;  // Enters widgets loop below.
                default:
                    this->dalLoggerAbort("Shouldn't reach here, the enum's index is: {}", static_cast<int>(ctrlFlag));

                }
            }

            for ( ; end != frontWidget; ++frontWidget ) {
                Widget2* w = *frontWidget;
                if ( !w->isPointInside(e.m_pos) ) {
                    continue;
                }

                const auto ctrlFlag = w->onTouch(e);
                switch ( ctrlFlag ) {

                case dal::InputCtrlFlag::consumed:
                    return { dal::InputCtrlFlag::consumed, w };
                case dal::InputCtrlFlag::ignored:
                    continue;
                case dal::InputCtrlFlag::owned:
                    state.m_owner = w;
                    return { dal::InputCtrlFlag::owned, w };
                default:
                    this->dalLoggerAbort("Shouldn't reach here, the enum's index is: {}", static_cast<int>(ctrlFlag));
                }
            }

            return { dal::InputCtrlFlag::ignored, nullptr };
        }

        template <typename _Iter>
        dal::InputCtrlFlag dispatch(_Iter frontWidget, const _Iter end, const KeyboardEvent& e, const KeyStatesRegistry& keyStates) {
            for ( ; end != frontWidget; ++frontWidget ) {
                const auto flag = (*frontWidget)->onKeyInput(e, keyStates);
                if ( dal::InputCtrlFlag::ignored != flag ) {
                    return flag;
                }
            }

            return dal::InputCtrlFlag::ignored;
        }

        void notifyWidgetRemoved(dal::Widget2* const w) {
            for ( auto& [id, state] : this->m_states ) {
                if ( w == state.m_owner ) {
                    state.m_owner = nullptr;
                    return;
                }
            }
        }

    private:
        TouchState& getOrMakeTouchState(const dal::touchID_t id, states_t& states) const {
            auto found = states.find(id);
            if ( this->m_states.end() != found ) {
                return found->second;
            }
            else {
                return states.emplace(id, TouchState{}).first->second;
            }
        }

        static void dalLoggerAbort(const char* const formatStr, int value);

    };

}