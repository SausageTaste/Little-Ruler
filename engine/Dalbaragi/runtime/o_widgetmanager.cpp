#include "o_widgetmanager.h"

#include <fmt/format.h>

#include <d_logger.h>


using namespace fmt::literals;


// Header functions
namespace dal {

    Widget2* resolveNewFocus(Widget2* const lastFocused, Widget2* const newlyFocused) {
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


// WidgetStorage
namespace dal {

    void WidgetStorage::addBack(Widget2* const w) {
        if ( !this->m_hasBackWidget ) {
            this->m_container.insert(this->m_container.begin(), w);
            this->m_hasBackWidget = true;
        }
        else {
            dalAbort("Tried to add back widget when where is already one.");
        }

    }

    /*
    void WidgetStorage::finalizeLoop(void) {
        if ( this->m_focusedStack.empty() ) {
            return;
        }

        std::vector<Widget2*> newContainer;
        {
            newContainer.reserve(this->m_container.size());

            const auto focusedStackEnd = this->m_focusedStack.end();
            for ( auto w : this->m_container ) {
                if ( focusedStackEnd == std::find(this->m_focusedStack.begin(), focusedStackEnd, w) ) {  // Not in focused list.
                    newContainer.push_back(w);
                }
            }

            for ( auto w : this->m_focusedStack ) {
                newContainer.push_back(w);
            }
        }

        if ( this->m_container.back() != newContainer.back() ) {
            this->m_container.back()->onFocusChange(false);
            newContainer.back()->onFocusChange(false);
        }

        this->m_container = std::move(newContainer);
    }
    */

    void WidgetStorage::setFocusOn(Widget2* const w) {
        if ( nullptr == w ) {
        }
        else if ( w == this->m_container.front() ) {  // When background got focus.
            if ( this->m_focusOnBG ) {
                return;  // Background widget is already on focus.
            }
            else {
                this->m_focusOnBG = true;
                this->m_container.front()->onFocusChange(true);
                this->m_container.back()->onFocusChange(false);
            }
        }
        else {  // When normal widget got focus.
            if ( this->m_focusOnBG ) {
                this->m_focusOnBG = false;
                this->m_container.front()->onFocusChange(false);
                this->remove(w);
                this->m_container.push_back(w);
                w->onFocusChange(true);
            }
            else {
                if ( this->m_container.back() == w ) {
                    return;
                }
                else {
                    this->m_container.back()->onFocusChange(false);
                    this->remove(w);
                    this->m_container.push_back(w);
                    w->onFocusChange(true);
                }
            }
        }
    }

}


// WidgetInputDispatcher
namespace dal {

    // Private

    void WidgetInputDispatcher::dalLoggerAbort(const char* const formatStr, int value) {
        dalAbort(fmt::format(formatStr, value));
    }

}