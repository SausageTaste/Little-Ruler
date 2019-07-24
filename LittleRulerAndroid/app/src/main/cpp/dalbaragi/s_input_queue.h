#pragma once

#include <array>
#include <string>

#include <glm/vec2.hpp>

#include "s_configs.h"


namespace dal {

    class ISingleUsageQueue {

        //////// vars ////////

    protected:
        static constexpr size_t kCapacity = 100;
        size_t mCurIndex = 0;

        //////// funcs ////////

    protected:
        ISingleUsageQueue(void) = default;
    public:
        virtual ~ISingleUsageQueue(void) = default;
    private:
        ISingleUsageQueue(const ISingleUsageQueue&) = delete;
        ISingleUsageQueue& operator=(const ISingleUsageQueue&) = delete;
        ISingleUsageQueue(ISingleUsageQueue&&) = delete;
        ISingleUsageQueue& operator=(ISingleUsageQueue&&) = delete;

    public:
        void clear(void);
        bool isFull(void) const;
        size_t getSize(void) const;
        size_t getCapacity(void) const;

    };

}


namespace dal {

    // -1 refers to null.
    using touchID_t = int32_t;

    enum class TouchActionType { down = 1, move = 2, up = 3 };


    struct TouchEvent {
        glm::vec2 m_pos;
        TouchActionType m_actionType;
        touchID_t m_id = -1;
        float m_timeInSec;
    };


    class TouchEvtQueueGod : public dal::ISingleUsageQueue {

        //////// vars ////////

    private:
        std::array<TouchEvent, kCapacity> mArray;

        //////// funcs ////////

    public:
        static TouchEvtQueueGod& getinst(void);

        bool emplaceBack(const float x, const float y, const TouchActionType type, const touchID_t id, const float timeSec);
        bool emplaceBack(const float x, const float y, const TouchActionType type, const touchID_t id);

        const TouchEvent& at(const unsigned int index) const;

    };

}


namespace dal {

    enum class KeySpec {
        /* unknown */
        unknown = 40,
        /* Alphabets */
        a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
        /* Vertical numbers */
        n0, n1, n2, n3, n4, n5, n6, n7, n8, n9,
        /* Misc in keyboard main area */
        backquote, minus, equal, lbracket, rbracket, backslash, semicolon, quote, comma, period, slash,
        /* Special characters */
        space, enter, backspace, tab,
        /* No characters */
        escape, lshfit, rshfit, lctrl, rctrl, lalt, ralt, up, down, left, right,
        /* EOF just to calculate number of elements of Enum class */
        eof
    };

    constexpr unsigned int KEY_SPEC_SIZE = int(KeySpec::eof) - int(KeySpec::unknown);

    char encodeKeySpecToAscii(const dal::KeySpec key, const bool shift);


    enum class KeyActionType { down, up };


    // TODO: Rename this to KeyActionType.
    struct KeyboardEvent {
        KeySpec m_key = KeySpec::unknown;
        KeyActionType m_actionType;
        float m_timeInSec;
    };


    class KeyStatesRegistry {

    public:
        struct KeyState {
            float m_lastUpdated = 0.0f;
            bool m_pressed = false;
        };

    private:
        static constexpr auto KEY_SPEC_SIZE_U = static_cast<unsigned int>(KeySpec::eof) - static_cast<unsigned int>(KeySpec::unknown);
        std::array<KeyState, KEY_SPEC_SIZE_U> m_states;

    public:
        void updateOne(const KeyboardEvent& e);

        KeyState& operator[](const KeySpec key) {
            return this->m_states[this->keySpecToIndex(key)];
        }

        const KeyState& operator[](const KeySpec key) const {
            return this->m_states[this->keySpecToIndex(key)];
        }

    private:
        static size_t keySpecToIndex(const KeySpec key) {
            return static_cast<size_t>(key) - static_cast<size_t>(KeySpec::unknown);
        }

    };


    class KeyboardEvtQueueGod : public dal::ISingleUsageQueue {

    private:
        std::array<KeyboardEvent, kCapacity> mArray;
        KeyStatesRegistry m_states;

    public:
        static KeyboardEvtQueueGod& getinst(void);

        bool emplaceBack(const KeySpec key, const KeyActionType type);
        bool emplaceBack(const KeySpec key, const KeyActionType type, const float timeSec);

        const KeyboardEvent& at(const unsigned int index) const;
        const KeyboardEvent& operator[](const unsigned int index) const {
            return this->mArray[index];
        }

        const KeyStatesRegistry& getKeyStates(void) const {
            return this->m_states;
        }

    };

}