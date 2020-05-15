#include "d_w_text_view.h"


namespace dal {

    Lable::Lable(Widget2D* const parent, overlayDrawFunc_t drawf, GlyphMaster& glyph)
        : Widget2D(parent, drawf)
        , m_text(this, glyph, drawf)
        , m_bg(this, drawf)
        , m_margin(0)
    {
        this->setTextColor(1, 1, 1);
        this->setBGColor(0, 0, 0);

        this->aabb().setPosSize(0, 0, 50, 20);
        this->onUpdateAABB();
    }

    void Lable::render(const float width, const float height, const void* userdata) {
        this->m_bg.render(width, height, userdata);
        this->m_text.render(width, height, userdata);
    }

    void Lable::onUpdateAABB(void) {
        this->m_bg.aabb().setAs(this->aabb());
        this->m_bg.onUpdateAABB();

        this->m_text.aabb().setPosSize(this->aabb().point00() + this->m_margin, this->aabb().size() - this->m_margin - this->m_margin);
        this->m_text.m_textSize = this->m_text.aabb().height();
        this->m_text.onUpdateAABB();
    }

}


namespace dal {

    auto LineEdit::onTouch(const TouchEvent& e) -> InputDealtFlag {
        if ( e.m_actionType == TouchActionType::down ) {
            return InputDealtFlag::consumed;
        }
        else {
            return InputDealtFlag::ignored;
        }
    }

    auto LineEdit::onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) -> InputDealtFlag {
        const auto shifted = keyStates[KeySpec::lshfit].m_pressed;

        if ( KeyActionType::down == e.m_actionType ) {
            const auto c = encodeKeySpecToAscii(e.m_key, shifted);

            switch ( c ) {

            case '\n':
                this->onReturn();
                break;
            case '\b':
                this->backspace();
                break;
            case '\0':
                break;
            default:
                this->addChar(c);
                break;

            }
        }

        return InputDealtFlag::consumed;
    }

    // Private

    void LineEdit::onReturn(void) {
        if ( this->m_onReturn ) {
            this->m_onReturn(this->text().c_str());
        }
        this->setText("");
    }

}


namespace dal {

    TextBox::TextBox(Widget2D* const parent, overlayDrawFunc_t drawf, GlyphMaster& glyph)
        : Widget2D(parent, drawf)
        , m_text(this, glyph, drawf)
        , m_bg(this, drawf)
        , m_margin(3)
    {
        this->m_text.m_color = glm::vec4{ 1, 1, 1, 1 };
        this->m_bg.m_color = glm::vec4{ 0, 0, 0, 1 };

        this->aabb().setPosSize(0, 0, 100, 100);
        this->onUpdateAABB();
    }

    void TextBox::render(const float width, const float height, const void* userdata) {
        this->m_bg.render(width, height, userdata);
        this->m_text.render(width, height, userdata);
    }

    void TextBox::onUpdateAABB(void) {
        this->m_bg.aabb().setAs(this->aabb());
        this->m_bg.onUpdateAABB();

        this->m_text.aabb().setPosSize(this->aabb().point00() + this->m_margin, this->aabb().size() - this->m_margin - this->m_margin);
        this->m_text.onUpdateAABB();
    }

    auto TextBox::onTouch(const TouchEvent& e) -> InputDealtFlag {
        if ( -1 != this->m_owning ) {
            if ( e.m_id == this->m_owning ) {
                if ( TouchActionType::move == e.m_actionType ) {
                    const auto rel = e.m_pos - this->m_lastTouchPos;
                    this->m_lastTouchPos = e.m_pos;
                    this->m_text.m_offset += rel;
                    return InputDealtFlag::owned;
                }
                else if ( TouchActionType::up == e.m_actionType ) {
                    const auto rel = e.m_pos - this->m_lastTouchPos;
                    this->m_lastTouchPos = e.m_pos;
                    this->m_text.m_offset += rel;

                    this->m_owning = -1;
                    return InputDealtFlag::consumed;
                }
                // If else ignores.
            }
            // If else ignores.
        }
        else {
            if ( TouchActionType::down == e.m_actionType ) {
                this->m_lastTouchPos = e.m_pos;
                this->m_owning = e.m_id;
                return InputDealtFlag::owned;
            }
            // If else ignores.
        }

        return InputDealtFlag::ignored;
    }

}