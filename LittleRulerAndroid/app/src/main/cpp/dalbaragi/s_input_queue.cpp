#include "s_input_queue.h"

#include "u_timer.h"
#include "s_logger_god.h"


using namespace std::string_literals;


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

    bool TouchEvtQueueGod::emplaceBack(const float x, const float y, const TouchType type, const int32_t id) {
        return this->emplaceBack(x, y, type, id, getTime_sec());
    }

    bool TouchEvtQueueGod::emplaceBack(const float x, const float y, const TouchType type, const int32_t id, const float timeSec) {
        if ( 0 > id || id >= 5 ) {
            dalWarn("Touch id is "s + std::to_string(id));
        }

        if ( !isFull() ) {
            mArray[mCurIndex].x = x;
            mArray[mCurIndex].y = y;
            mArray[mCurIndex].type = type;
            mArray[mCurIndex].id = id;
            mArray[mCurIndex].timeSec = timeSec;
            mCurIndex++;
            return true;
        }
        else {
            dalWarn("TouchEvtQueueGod is full");
            return false;
        }
    }

    const TouchEvent& TouchEvtQueueGod::at(const unsigned int index) const {
        return mArray.at(index);
    }

}


namespace dal {

    KeyboardEvtQueueGod& KeyboardEvtQueueGod::getinst(void) {
        static KeyboardEvtQueueGod inst;
        return inst;
    }

    bool KeyboardEvtQueueGod::emplaceBack(const KeySpec key, const KeyboardType type) {
        return this->emplaceBack(key, type, getTime_sec());
    }

    bool KeyboardEvtQueueGod::emplaceBack(const KeySpec key, const KeyboardType type, const float timeSec) {
        if ( isFull() ) {
            dalWarn("KeyboardEvtQueueGod is full");
            return false;
        }

        mArray[mCurIndex].key = key;
        mArray[mCurIndex].type = type;
        mArray[mCurIndex].timeSec = timeSec;
        mCurIndex++;
        return true;
    }

    const KeyboardEvent& KeyboardEvtQueueGod::at(const unsigned int index) const {
        return mArray.at(index);
    }

}