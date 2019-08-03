#pragma once

#include <vector>


namespace dal {

    class IFiniteState {

    public:
        virtual ~IFiniteState(void) = 0;
        virtual IFiniteState* exec(void) = 0;

    };


    class FiniteStateMachine {

    private:
        IFiniteState* m_currentState = nullptr;

    public:
        void exec(void) {
            this->m_currentState = this->m_currentState->exec();
        }

    };

}


namespace dal {

    class IPlayerState : public IFiniteState {

    public:


    };

}