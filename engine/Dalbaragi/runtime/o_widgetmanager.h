#pragma once

#include <vector>

#include "o_widgetbase.h"


namespace dal {

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

}
