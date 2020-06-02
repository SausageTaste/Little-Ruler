#include "c_input_apply.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>
#include <d_overlay_interface.h>

#include "u_math.h"


using namespace fmt::literals;


// MoveDPad
namespace dal {

    PlayerControlWidget::MoveDPad::MoveDPad(dal::Widget2D* const parent, const float winWidth, const float winHeight)
        : Widget2D(parent, drawOverlay)
        , m_fixedCenterPoint(this, drawOverlay)
        , m_touchedPoint(this, drawOverlay)
    {
        this->onParentResize(winWidth, winHeight);

        this->m_fixedCenterPoint.m_color = glm::vec4{ 1, 1, 1, 1 };
        this->m_touchedPoint.m_color = glm::vec4{ 1, 1, 1, 1 };
        this->setSquareLength(20);
    }


    void PlayerControlWidget::MoveDPad::render(const float width, const float height, const void* uniloc) {
        this->m_fixedCenterPoint.render(width, height, uniloc);

        if ( -1 != this->m_owning ) {
            this->m_touchedPoint.aabb().pos() = this->m_touchedPos - this->m_squareLengthHalf;
            this->m_touchedPoint.render(width, height, uniloc);
        }
    }

    InputDealtFlag PlayerControlWidget::MoveDPad::onTouch(const dal::TouchEvent& e) {
        if ( this->isActive() ) {
            if ( e.m_id == this->m_owning ) {
                if ( e.m_actionType == TouchActionType::move ) {
                    this->updateTouchedPos(e.m_pos.x, e.m_pos.y, this->m_touchedPos);
                    return InputDealtFlag::owned;
                }
                else if ( TouchActionType::up == e.m_actionType ) {
                    this->m_owning = -1;
                    return InputDealtFlag::consumed;
                }
                else {
                    // Else case is when event type is down, which shouldn't happen.
                    // But it's really Android system who is responsible for that so I can't assure.
                    dalWarn("Got touch event type down for the id which was already being processed as down.");
                    return InputDealtFlag::ignored;
                }
            }
            // If else ignores.
        }
        else {  // If not active.
            if ( TouchActionType::down == e.m_actionType && this->isInsideCircle(e.m_pos.x, e.m_pos.y) ) {
                // Start owning the touch id.
                this->m_owning = e.m_id;
                this->updateTouchedPos(e.m_pos.x, e.m_pos.y, this->m_touchedPos);
                return InputDealtFlag::owned;
            }
            // If else ignores.
        }

        return InputDealtFlag::ignored;
    }


    void PlayerControlWidget::MoveDPad::onParentResize(const float width, const float height) {
        const auto shorter = width < height ? width : height;
        const auto edgeLen = shorter * 0.5f;
        this->aabb().setPosSize(CORNER_MARGIN, height - edgeLen - CORNER_MARGIN, edgeLen, edgeLen);

        this->m_fixedCenterPoint.aabb().pos() = this->makeFixedCenterPos() - this->m_squareLengthHalf;
    }

    void PlayerControlWidget::MoveDPad::setSquareLength(const float x) {
        this->m_squareLengthHalf = x * 0.5f;

        this->m_fixedCenterPoint.aabb().pos() = this->makeFixedCenterPos() - this->m_squareLengthHalf;
        this->m_fixedCenterPoint.aabb().size() = glm::vec2{ this->m_squareLengthHalf * 2.f, this->m_squareLengthHalf * 2.f };
        this->m_fixedCenterPoint.onUpdateDimens(1);

        this->m_touchedPoint.aabb().size() = glm::vec2{ this->m_squareLengthHalf * 2.f, this->m_squareLengthHalf * 2.f };
        this->m_touchedPoint.onUpdateDimens(1);
    }


    glm::vec2 PlayerControlWidget::MoveDPad::getRel(void) const {
        if ( !this->isActive() ) {
            return glm::vec2{ 0.0f };
        }

        dalAssert(this->aabb().width() == this->aabb().height());

        const auto fixedCenter = this->makeFixedCenterPos();
        return (this->m_touchedPos - fixedCenter) / (this->aabb().width() * 0.5f);
    }

    bool PlayerControlWidget::MoveDPad::isActive(void) const {
        return -1 != this->m_owning;
    }

    // Private

    void PlayerControlWidget::MoveDPad::updateTouchedPos(const float x, const float y, glm::vec2& target) const {
        dalAssert(this->aabb().width() == this->aabb().height());

        const auto fixedCenter = this->makeFixedCenterPos();;
        target = clampVec(glm::vec2{ x, y } -fixedCenter, this->aabb().width() * 0.5f) + fixedCenter;
    }

    glm::vec2 PlayerControlWidget::MoveDPad::makeFixedCenterPos(void) const {
        return glm::vec2{
            this->aabb().pos().x + this->aabb().width() * 0.5f,
            this->aabb().pos().y + this->aabb().height() * 0.5f
        };
    }

    bool PlayerControlWidget::MoveDPad::isInsideCircle(const glm::vec2& v) const {
        const float radiusSqr = this->aabb().width() * this->aabb().width() * 0.5f * 0.5f;
        const auto center = this->makeFixedCenterPos();
        const auto rel = v - center;
        const auto lenSqr = glm::dot(rel, rel);
        return lenSqr < radiusSqr;
    }

}


// PlayerControlWidget
namespace dal {

    PlayerControlWidget::PlayerControlWidget(const float winWidth, const float winHeight)
        : Widget2D(nullptr, drawOverlay)
        , m_dpad(this, winWidth, winHeight)
        , m_owningForView(-1)
    {
        this->aabb().setPosSize<float>(0, 0, winWidth, winHeight);
    }

    void PlayerControlWidget::render(const float width, const float height, const void* uniloc) {
        this->m_dpad.render(width, height, uniloc);
    }

    InputDealtFlag PlayerControlWidget::onTouch(const TouchEvent& e) {
        const auto dpadReturnFlag = this->m_dpad.onTouch(e);

        if ( InputDealtFlag::ignored != dpadReturnFlag ) {
            return dpadReturnFlag;
        }
        else {
            if ( -1 != this->m_owningForView ) {
                if ( e.m_id == this->m_owningForView ) {
                    if ( e.m_actionType == TouchActionType::move ) {
                        this->m_viewTouchAccum = e.m_pos - this->m_lastViewTouchPos;
                        this->m_lastViewTouchPos = e.m_pos;
                        return InputDealtFlag::owned;
                    }
                    else if ( TouchActionType::up == e.m_actionType ) {
                        this->m_owningForView = -1;
                        return InputDealtFlag::consumed;
                    }
                    else {
                        // Else case is when event type is down, which shouldn't happen.
                        // But it's really Android system who is responsible for that so I can't assure.
                        dalWarn("Got touch event type down for the id which was already being processed as down.");
                        return InputDealtFlag::ignored;
                    }
                }
                // If else ignores.
            }
            else {  // If not active.
                if ( TouchActionType::down == e.m_actionType ) {
                    // Start owning the touch id.
                    this->m_owningForView = e.m_id;
                    this->m_lastViewTouchPos = e.m_pos;
                    this->m_viewTouchAccum = glm::vec2{ 0.0f };
                    return InputDealtFlag::owned;
                }
                // If else ignores.
            }

            return InputDealtFlag::ignored;
        }
    }

    InputDealtFlag PlayerControlWidget::onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) {
        this->m_keyboardMoveInfo.clear();

        if ( keyStates[dal::KeySpec::w].m_pressed ) {
            this->m_keyboardMoveInfo.m_move.y -= 1;
        }
        if ( keyStates[(dal::KeySpec::s)].m_pressed ) {
            this->m_keyboardMoveInfo.m_move.y += 1;
        }
        if ( keyStates[(dal::KeySpec::a)].m_pressed ) {
            this->m_keyboardMoveInfo.m_move.x -= 1;
        }
        if ( keyStates[(dal::KeySpec::d)].m_pressed ) {
            this->m_keyboardMoveInfo.m_move.x += 1;
        }
        if ( keyStates[(dal::KeySpec::space)].m_pressed ) {
            this->m_keyboardMoveInfo.m_jump = true;
        }

        constexpr float viewMultiplier = 1.0f;

        if ( keyStates[(dal::KeySpec::left)].m_pressed ) {
            this->m_keyboardMoveInfo.m_view.x -= viewMultiplier;
        }
        if ( keyStates[(dal::KeySpec::right)].m_pressed ) {
            this->m_keyboardMoveInfo.m_view.x += viewMultiplier;
        }
        if ( keyStates[(dal::KeySpec::up)].m_pressed ) {
            this->m_keyboardMoveInfo.m_view.y += viewMultiplier;
        }
        if ( keyStates[(dal::KeySpec::down)].m_pressed ) {
            this->m_keyboardMoveInfo.m_view.y -= viewMultiplier;
        }

        if ( this->m_keyboardMoveInfo.m_move.x != 0.0f || this->m_keyboardMoveInfo.m_move.y != 0.0f ) {
            this->m_keyboardMoveInfo.m_move = glm::normalize(this->m_keyboardMoveInfo.m_move);
        }

        return InputDealtFlag::consumed;
    }

    void PlayerControlWidget::onFocusChange(const bool v) {
        if ( v ) {
            this->m_keyboardMoveInfo.clear();
        }
    }


    MoveInputInfo PlayerControlWidget::getMoveInfo(const float deltaTime, const float winWidth, const float winHeight) {
        MoveInputInfo info;

        const float widthOrHeightButShorter = winWidth < winHeight ? winWidth : winHeight;
        const float viewMultiplier = 5.0f / widthOrHeightButShorter;

        {
            info.merge(this->m_keyboardMoveInfo);
            info.m_view *= deltaTime * 2.0f;
        }

        {
            info.m_move += this->getMoveVec();
        }

        {
            const auto rel = this->getResetViewAccum();
            info.m_view.x += rel.x * viewMultiplier;
            info.m_view.y += -rel.y * viewMultiplier;
        }

        return info;
    }

    void PlayerControlWidget::onParentResize(const float width, const float height) {
        this->m_dpad.onParentResize(width, height);

        this->aabb().setPosSize<float>(0, 0, width, height);
    }

    // Private

    glm::vec2 PlayerControlWidget::getMoveVec(void) const {
        return this->m_dpad.getRel();
    }

    glm::vec2 PlayerControlWidget::getResetViewAccum(void) {
        const auto tmp = this->m_viewTouchAccum;
        this->m_viewTouchAccum = glm::vec2{ 0.0f };
        return tmp;
    }

}
