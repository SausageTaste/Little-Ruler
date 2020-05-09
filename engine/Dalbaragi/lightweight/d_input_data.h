#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>


namespace dal {

    // -1 refers to null.
    using touchID_t = std::int32_t;

    enum class TouchActionType { down = 1, move = 2, up = 3 };

    struct TouchEvent {
        glm::vec2 m_pos{ 0 };
        TouchActionType m_actionType = TouchActionType::down;
        touchID_t m_id = -1;
        float m_timeInSec = 0.f;
    };


    enum class KeyActionType { down, up };

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

    // TODO: Rename this to KeyActionType.
    struct KeyboardEvent {
        KeySpec m_key = KeySpec::unknown;
        KeyActionType m_actionType = KeyActionType::down;
        float m_timeInSec = 0.f;
    };

    class KeyStatesRegistry {

    public:
        struct KeyState {
            float m_lastUpdated = 0.f;
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

}
