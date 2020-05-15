#include "d_input_data.h"


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
