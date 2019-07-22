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

    bool TouchEvtQueueGod::emplaceBack(const float x, const float y, const TouchType type, const touchID_t id) {
        return this->emplaceBack(x, y, type, id, getTime_sec());
    }

    bool TouchEvtQueueGod::emplaceBack(const float x, const float y, const TouchType type, const touchID_t id, const float timeSec) {
        if ( 0 > id || id >= 5 ) {
            dalWarn("Touch id is "s + std::to_string(id));
        }
        if ( -1 == id ) {
            dalWarn("Touch id -1 is for null.");
            return false;
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
        try {
            return this->mArray.at(index);
        }
        catch ( const std::out_of_range& e ) {
            dalAbort("Out of range exception thrown.");
        }
    }

}


namespace dal {

    char encodeKeySpecToAscii(const dal::KeySpec key, const bool shift) {
        const auto keyInt = static_cast<int>(key);

        if ( static_cast<int>(dal::KeySpec::a) <= keyInt && keyInt <= static_cast<int>(dal::KeySpec::z) ) {
            if ( shift ) {
                return static_cast<char>(static_cast<int>('A') + keyInt - static_cast<int>(dal::KeySpec::a));
            }
            else {
                return char(static_cast<int>('a') + keyInt - static_cast<int>(dal::KeySpec::a));
            }
        }
        else if ( static_cast<int>(dal::KeySpec::n0) <= keyInt && keyInt <= static_cast<int>(dal::KeySpec::n9) ) {
            if ( shift ) {
                const auto index = keyInt - static_cast<int>(dal::KeySpec::n0);
                constexpr char map[] = { ')','!','@','#','$','%','^','&','*','(' };
                return map[index];
            }
            else {
                return static_cast<char>(static_cast<int>('0') + keyInt - static_cast<int>(dal::KeySpec::n0));
            }
        }
        else if ( static_cast<int>(dal::KeySpec::backquote) <= keyInt && keyInt <= static_cast<int>(dal::KeySpec::slash) ) {
            // backquote, minus, equal, lbracket, rbracket, backslash, semicolon, quote, comma, period, slash
            const auto index = keyInt - static_cast<int>(dal::KeySpec::backquote);
            if ( shift ) {
                constexpr char map[] = { '~', '_', '+', '{', '}', '|', ':', '"', '<', '>', '?' };
                return map[index];
            }
            else {
                constexpr char map[] = { '`', '-', '=', '[', ']', '\\', ';', '\'', ',', '.', '/' };
                return map[index];
            }
        }
        else if ( static_cast<int>(dal::KeySpec::space) <= keyInt && keyInt <= static_cast<int>(dal::KeySpec::tab) ) {
            // space, enter, backspace, tab
            const auto index = keyInt - static_cast<int>(dal::KeySpec::space);
            constexpr char map[] = { ' ', '\n', '\b', '\t' };
            return map[index];
        }
        else {
            return '\0';
        }
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