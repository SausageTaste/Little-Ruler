#include "o_widget_textbox.h"

#include <cstring>
#include <string>

#include <fmt/format.h>

#include <d_logger.h>

#include "s_configs.h"


using namespace fmt::literals;


namespace {

    constexpr float DARK_THEME_COLOR = 30.0f / 255.0f;

}


// Label2
namespace dal {

    Label2::Label2(Widget2* parent)
        : Widget2(parent)
        , m_textRenderer(this)
        , m_backgroundColor(DARK_THEME_COLOR, DARK_THEME_COLOR, DARK_THEME_COLOR, 1.0f)
    {

    }

    void Label2::render(const UnilocOverlay& uniloc, const float width, const float height) {
        QuadRenderInfo info;
        std::tie(info.m_bottomLeftNormalized, info.m_rectSize) = this->makePosSize(width, height);
        info.m_color = this->m_backgroundColor;
        renderQuadOverlay(uniloc, info);

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
                if (!text.textbuf().empty() ) {
                    text.textbuf().pop_back();
                }
                break;
            case '\0':
                break;
            default:
                text.textbuf() += c;
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
            auto str = this->m_strBuf->data();
            this->m_textRenderer.textbuf() += str;
            this->m_strBuf->clear();
        }

        QuadRenderInfo info;
        std::tie(info.m_bottomLeftNormalized, info.m_rectSize) = this->makePosSize(width, height);
        info.m_color = glm::vec4{ DARK_THEME_COLOR, DARK_THEME_COLOR, DARK_THEME_COLOR, 1.0f };
        renderQuadOverlay(uniloc, info);

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
