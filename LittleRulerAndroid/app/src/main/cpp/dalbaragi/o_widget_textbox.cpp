#include "o_widget_textbox.h"

#include <cstring>
#include <string>

#include <fmt/format.h>

#include "s_logger_god.h"
#include "m_collider.h"
#include "s_scripting.h"
#include "p_globalfsm.h"
#include "s_configs.h"


using namespace std::string_literals;
using namespace fmt::literals;


namespace {

    auto& g_logger = dal::LoggerGod::getinst();

    constexpr float g_darkTheme = 30.0f / 255.0f;


    void toggleGameState(void) {
        dal::EventStatic e;
        e.type = dal::EventType::global_fsm_change;

        const auto curState = dal::ConfigsGod::getinst().getGlobalGameState();

        switch ( curState ) {
        case dal::GlobalGameState::game:
            e.intArg1 = static_cast<int>(dal::GlobalGameState::menu); break;
        case dal::GlobalGameState::menu:
            e.intArg1 = static_cast<int>(dal::GlobalGameState::game); break;
        }

        dal::EventGod::getinst().notifyAll(e);
    }

}


namespace {

    // from https://github.com/s22h/cutils/blob/master/include/s22h/unicode.h

    constexpr uint32_t UTF8_ONE_BYTE_MASK = 0x80;
    constexpr uint32_t UTF8_ONE_BYTE_BITS = 0;
    constexpr uint32_t UTF8_TWO_BYTES_MASK = 0xE0;
    constexpr uint32_t UTF8_TWO_BYTES_BITS = 0xC0;
    constexpr uint32_t UTF8_THREE_BYTES_MASK = 0xF0;
    constexpr uint32_t UTF8_THREE_BYTES_BITS = 0xE0;
    constexpr uint32_t UTF8_FOUR_BYTES_MASK = 0xF8;
    constexpr uint32_t UTF8_FOUR_BYTES_BITS = 0xF0;
    constexpr uint32_t UTF8_CONTINUATION_MASK = 0xC0;
    constexpr uint32_t UTF8_CONTINUATION_BITS = 0x80;

    size_t utf8_codepoint_size(const uint8_t byte) {
        if ( (byte & UTF8_ONE_BYTE_MASK) == UTF8_ONE_BYTE_BITS ) {
            return 1;
        }

        if ( (byte & UTF8_TWO_BYTES_MASK) == UTF8_TWO_BYTES_BITS ) {
            return 2;
        }

        if ( (byte & UTF8_THREE_BYTES_MASK) == UTF8_THREE_BYTES_BITS ) {
            return 3;
        }

        if ( (byte & UTF8_FOUR_BYTES_MASK) == UTF8_FOUR_BYTES_BITS ) {
            return 4;
        }

        return 0;
    }

    uint32_t convert_utf8_to_utf32(const uint8_t* begin, const uint8_t* end) {
        const auto cp_size = end - begin;
        assert((utf8_codepoint_size(*begin)) == cp_size);
        assert(1 <= cp_size && cp_size <= 4);

        uint32_t buffer = 0;

        switch ( cp_size ) {

        case 1:
            buffer = ((uint32_t)begin[0] & ~UTF8_ONE_BYTE_MASK);
            return buffer;
        case 2:
            buffer =
                ((uint32_t)begin[0] & ~UTF8_TWO_BYTES_MASK) << 6 |
                ((uint32_t)begin[1] & ~UTF8_CONTINUATION_MASK);
            return buffer;
        case 3:
            buffer =
                ((uint32_t)begin[0] & ~UTF8_THREE_BYTES_MASK) << 12 |
                ((uint32_t)begin[1] & ~UTF8_CONTINUATION_MASK) << 6 |
                ((uint32_t)begin[2] & ~UTF8_CONTINUATION_MASK);
            return buffer;
        case 4:
            buffer =
                ((uint32_t)begin[0] & ~UTF8_FOUR_BYTES_MASK) << 18 |
                ((uint32_t)begin[1] & ~UTF8_CONTINUATION_MASK) << 12 |
                ((uint32_t)begin[2] & ~UTF8_CONTINUATION_MASK) << 6 |
                ((uint32_t)begin[3] & ~UTF8_CONTINUATION_MASK);
            return buffer;
        default:
            // this should never happen, since we check validity of the string
            fprintf(stderr, "utf8_to_utf32: invalid byte in UTF8 string.\n");
            return 0;

        }
    }

}


// Label2
namespace dal {

    Label2::Label2(Widget2* parent)
        : Widget2(parent)
        , m_textRenderer(this)
        , m_backgroundColor(g_darkTheme, g_darkTheme, g_darkTheme, 1.0f)
    {

    }

    void Label2::render(const UnilocOverlay& uniloc, const float width, const float height) {
        const auto deviceSpace = this->makeDeviceSpace(width, height);
        renderQuadOverlay(uniloc, deviceSpace.first, deviceSpace.second, this->m_backgroundColor);

        this->m_textRenderer.render(uniloc, width, height);
    }

    // Protected

    void Label2::onScrSpaceBoxUpdate(void) {
        this->m_textRenderer.setPos(this->getPos());
        this->m_textRenderer.setSize(this->getSize());
    }

}


namespace dal {

    LineEdit::LineEdit(Widget2* const parent)
        : Label2(parent)
        , m_onFocus(false)
    {

    }

    void LineEdit::render(const UnilocOverlay& uniloc, const float width, const float height) {
        if ( this->m_onFocus ) {
            this->getTextRenderer().setCursorPos(this->getText().size() - 1);
        }
        else {
            this->getTextRenderer().setCursorPos(TextRenderer::cursorNullPos);
        }

        Label2::render(uniloc, width, height);
    }

    InputCtrlFlag LineEdit::onTouch(const TouchEvent& e) {
        if ( e.m_actionType == TouchActionType::down ) {
            return InputCtrlFlag::consumed;
        }
        else {
            return InputCtrlFlag::ignored;
        }
    }

    InputCtrlFlag LineEdit::onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) {
        const auto shifted = keyStates[KeySpec::lshfit].m_pressed;

        if ( KeyActionType::down == e.m_actionType ) {
            auto& text = this->getTextRenderer();
            const auto c = encodeKeySpecToAscii(e.m_key, shifted);

            switch ( c ) {

            case '\n':
                this->onReturn();
                break;
            case '\b':
                if (!text.getText().empty() ) {
                    text.popBackText();
                }
                break;
            case '\0':
                break;
            default:
                text.appendText(c);
                break;

            }
        }

        return InputCtrlFlag::consumed;
    }

    void LineEdit::onFocusChange(const bool v) {
        this->m_onFocus = v;
    }

    // Protected

    void LineEdit::onScrSpaceBoxUpdate(void) {
        this->getTextRenderer().setPos(this->getPos());
        this->getTextRenderer().setSize(this->getSize());
    }

    // Private

    void LineEdit::onReturn(void) {
        if ( this->m_callbackOnEnter ) {
            this->m_callbackOnEnter(this->getText().c_str());
        }
        this->setText("");
    }

}


namespace dal {

    TextBox::TextBox(Widget2* const parent)
        : Widget2(parent)
        , m_textRenderer(this)
        , m_strBuf(nullptr)
    {

    }

    void TextBox::render(const UnilocOverlay& uniloc, const float width, const float height) {
        if ( nullptr != this->m_strBuf ) {
            auto str = this->m_strBuf->getStrBuf();
            this->m_textRenderer.appendText(str);
            this->m_strBuf->clear();
        }

        const auto info = this->makeDeviceSpace(width, height);
        renderQuadOverlay(uniloc, info.first, info.second, glm::vec4{ g_darkTheme, g_darkTheme, g_darkTheme, 1.0f });

        this->m_textRenderer.render(uniloc, width, height);
    }

    InputCtrlFlag TextBox::onTouch(const TouchEvent& e) {
        return this->m_textRenderer.onTouch(e);
    }

    StringBufferBasic* TextBox::replaceBuffer(StringBufferBasic* const buffer) {
        const auto tmp = this->m_strBuf;
        this->m_strBuf = buffer;
        return tmp;
    }

    // Protected

    void TextBox::onScrSpaceBoxUpdate(void) {
        this->m_textRenderer.copy(*this);
    }

}