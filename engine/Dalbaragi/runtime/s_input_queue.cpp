#include "s_input_queue.h"

#include <spdlog/fmt/fmt.h>

#include <d_logger.h>

#include "u_timer.h"


using namespace fmt::literals;


namespace dal {

    bool ISingleUsageQueue::isFull(void) const {
        return mCurIndex >= kCapacity;
    }

    void ISingleUsageQueue::clear(void) {
        mCurIndex = 0;
    }

    size_t ISingleUsageQueue::getSize(void) const {
        return mCurIndex;
    }

    size_t ISingleUsageQueue::getCapacity(void) const {
        return kCapacity;
    }

}


namespace dal {

    TouchEvtQueueGod& TouchEvtQueueGod::getinst(void) {
        static TouchEvtQueueGod inst;
        return inst;
    }

    bool TouchEvtQueueGod::emplaceBack(const float x, const float y, const TouchActionType type, const touchID_t id) {
        return this->emplaceBack(x, y, type, id, getTime_sec());
    }

    bool TouchEvtQueueGod::emplaceBack(const float x, const float y, const TouchActionType type, const touchID_t id, const float timeSec) {
        if ( 0 > id || id >= 5 ) {
            dalWarn(fmt::format("Touch id is \"{}\"", id));
        }
        if ( -1 == id ) {
            dalWarn("Touch id -1 is for null.");
            return false;
        }

        if ( !isFull() ) {
            mArray[mCurIndex].m_pos.x = x;
            mArray[mCurIndex].m_pos.y = y;
            mArray[mCurIndex].m_actionType = type;
            mArray[mCurIndex].m_id = id;
            mArray[mCurIndex].m_timeInSec = timeSec;
            mCurIndex++;
            return true;
        }
        else {
            dalWarn("TouchEvtQueueGod is full");
            return false;
        }
    }

    const TouchEvent& TouchEvtQueueGod::at(const unsigned int index) const {
        try {
            return this->mArray.at(index);
        }
        catch ( const std::out_of_range& e ) {
            dalAbort(fmt::format("Out of range exception thrown: {}", e.what()));
        }
    }

}


namespace dal {

    void KeyStatesRegistry::updateOne(const KeyboardEvent& e) {
        const auto index = this->keySpecToIndex(e.m_key);
        this->m_states[index].m_lastUpdated = e.m_timeInSec;
        this->m_states[index].m_pressed = (e.m_actionType == KeyActionType::down);
    }


    KeyboardEvtQueueGod& KeyboardEvtQueueGod::getinst(void) {
        static KeyboardEvtQueueGod inst;
        return inst;
    }

    bool KeyboardEvtQueueGod::emplaceBack(const KeySpec key, const KeyActionType type) {
        return this->emplaceBack(key, type, getTime_sec());
    }

    bool KeyboardEvtQueueGod::emplaceBack(const KeySpec key, const KeyActionType type, const float timeSec) {
        if ( isFull() ) {
            dalWarn("KeyboardEvtQueueGod is full");
            return false;
        }

        this->mArray[mCurIndex].m_key = key;
        this->mArray[mCurIndex].m_actionType = type;
        this->mArray[mCurIndex].m_timeInSec = timeSec;

        this->m_states.updateOne(this->mArray[this->mCurIndex]);

        this->mCurIndex++;

        return true;
    }

    const KeyboardEvent& KeyboardEvtQueueGod::at(const unsigned int index) const {
        try {
            return this->mArray.at(index);
        }
        catch ( const std::out_of_range& e ) {
            dalAbort(fmt::format("Out of range exception thrown: {}", e.what()));
        }
    }

}
