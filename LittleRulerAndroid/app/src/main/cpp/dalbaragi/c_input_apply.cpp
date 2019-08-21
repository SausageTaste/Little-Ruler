#include "c_input_apply.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"
#include "u_vecutil.h"


using namespace fmt::literals;


namespace {

    void toggleGameState(void) {
        dal::EventStatic e;
        e.type = dal::EventType::global_fsm_change;

        const auto curState = dal::GlobalStateGod::getinst().getGlobalGameState();
        switch ( curState ) {
        case dal::GlobalGameState::game:
            e.intArg1 = static_cast<int>(dal::GlobalGameState::menu); break;
        case dal::GlobalGameState::menu:
            e.intArg1 = static_cast<int>(dal::GlobalGameState::game); break;
        }

        dal::EventGod::getinst().notifyAll(e);
    }

    inline constexpr unsigned int enum2Index(const dal::KeySpec key) {
        return static_cast<unsigned int>(key) - static_cast<unsigned int>(dal::KeySpec::unknown);
    }

    inline constexpr dal::KeySpec index2Enum(const unsigned int index) {
        return static_cast<dal::KeySpec>(index + static_cast<unsigned int>(dal::KeySpec::unknown));
    }

}  // namespace


namespace {

    /*
    void apply_flyDirectional(const float deltaTime, const dal::MoveInputInfo& totalMoveInfo, dal::StrangeEulerCamera& camera) {
        camera.addViewPlane(totalMoveInfo.m_view.x, totalMoveInfo.m_view.y);

        // Apply move direction if target need to move.
        if ( totalMoveInfo.m_move.x != 0.0f || totalMoveInfo.m_move.y != 0.0f || totalMoveInfo.m_vertical != 0.0f ) {
            glm::mat4 viewMat{ 1.0 };
            const auto viewVec = camera.getViewPlane();
            viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
            viewMat = glm::rotate(viewMat, viewVec.y, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::vec3 moveDirection{ viewMat * glm::vec4{totalMoveInfo.m_move.x, 0.0, totalMoveInfo.m_move.y, 1.0} };
            if ( totalMoveInfo.m_vertical ) {
                moveDirection.y = totalMoveInfo.m_vertical;
            }

            //moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
            const float moveMultiplier = 5.0f * deltaTime;
            camera.m_pos += moveDirection * moveMultiplier;
        }

        camera.updateViewMat();
    }

    void apply_flyPlane(const float deltaTime, const dal::MoveInputInfo& totalMoveInfo, dal::StrangeEulerCamera& camera) {
        // Apply view
        camera.addViewPlane(totalMoveInfo.m_view.x, totalMoveInfo.m_view.y);

        // Apply move direction if target need to move.
        if ( totalMoveInfo.m_move.x != 0.0f || totalMoveInfo.m_move.y != 0.0f || totalMoveInfo.m_vertical != 0.0f ) {
            glm::mat4 viewMat{ 1.0 };
            const auto viewVec = camera.getViewPlane();
            viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
            //viewMat = glm::rotate(viewMat, targetViewDir->y, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::vec3 moveDirection{ viewMat * glm::vec4{totalMoveInfo.m_move.x, 0.0, totalMoveInfo.m_move.y, 1.0} };
            moveDirection.y = totalMoveInfo.m_vertical;

            //moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
            const float moveMultiplier = 5.0f * deltaTime;
            camera.m_pos += moveDirection * moveMultiplier;
            camera.m_pos.y += moveDirection.y * 50.0f * deltaTime;
        }

        camera.updateViewMat();
    }

    void apply_flyForPlatform(const float deltaTime, const dal::MoveInputInfo& totalMoveInfo, dal::StrangeEulerCamera& camera) {
#if defined(_WIN32)
        apply_flyPlane(deltaTime, totalMoveInfo, camera);
#elif defined(__ANDROID__)
        apply_flyDirectional(deltaTime, totalMoveInfo, camera);
#endif
    }
    */

    /*
    void apply_topdown(const float deltaTime, const dal::MoveInputInfo& totalMoveInfo,
        dal::StrangeEulerCamera& camera, const entt::entity targetEntity, entt::registry& reg)
    {
        if ( reg.has<dal::cpnt::AnimatedModel>(targetEntity) ) {
            reg.get<dal::cpnt::AnimatedModel>(targetEntity).m_animState.setSelectedAnimeIndex(0);
        }

        // Apply move direction
        {
            const glm::vec3 k_modelCamOffset{ 0.0f, 1.3f, 0.0f };

            dalAssert(reg.has<dal::cpnt::Transform>(targetEntity));
            auto& cpntTrans = reg.get<dal::cpnt::Transform>(targetEntity);

            {
                constexpr float CAM_ROTATE_SPEED_INV = 1.0f;
                static_assert(0.0f <= CAM_ROTATE_SPEED_INV && CAM_ROTATE_SPEED_INV <= 1.0f);

                const auto camViewVec = dal::strangeEuler2Vec(camera.getStrangeEuler());
                const auto rotatorAsCamX = glm::rotate(glm::mat4{ 1.0f }, camera.getStrangeEuler().getX(), glm::vec3{ 0.0f, 1.0f, 0.0f });
                const auto rotatedMoveVec = dal::rotateVec2(glm::vec2{ totalMoveInfo.m_move.x, totalMoveInfo.m_move.y }, camera.getStrangeEuler().getX());

                const auto deltaPos = glm::vec3{ rotatedMoveVec.x, totalMoveInfo.m_vertical, rotatedMoveVec.y } *deltaTime * 5.0f;
                cpntTrans.m_pos += deltaPos;
                camera.m_pos += deltaPos * CAM_ROTATE_SPEED_INV;
                if ( rotatedMoveVec.x != 0.0f || rotatedMoveVec.y != 0.0f ) {  // If moved position
                    cpntTrans.m_quat = dal::rotateQuat(glm::quat{}, atan2(rotatedMoveVec.x, rotatedMoveVec.y), glm::vec3{ 0.0f, 1.0f, 0.0f });
                    if ( reg.has<dal::cpnt::AnimatedModel>(targetEntity) ) {
                        auto& animModel = reg.get<dal::cpnt::AnimatedModel>(targetEntity);

                        animModel.m_animState.setSelectedAnimeIndex(1);

                        const auto moveSpeed = glm::length(rotatedMoveVec);
                        const auto animeSpeed = 1.0f / moveSpeed;
                        constexpr float epsilon = 0.0001f;
                        if ( epsilon > animeSpeed && animeSpeed > -epsilon ) {  // animeSpeed is near zero than epsion.
                            animModel.m_animState.setTimeScale(0.0f);
                        }
                        else {
                            animModel.m_animState.setTimeScale(1.0f / animeSpeed);
                        }
                    }
                }
                else {
                    if ( reg.has<dal::cpnt::AnimatedModel>(targetEntity) ) {
                        auto& animModel = reg.get<dal::cpnt::AnimatedModel>(targetEntity);
                        animModel.m_animState.setSelectedAnimeIndex(0);
                    }
                }
            }

            {
                constexpr float MAX_Y_DEGREE = 75.0f;

                const auto obj2CamVec = camera.m_pos - (cpntTrans.m_pos + k_modelCamOffset);
                const auto len = glm::length(obj2CamVec);
                auto obj2CamSEuler = dal::vec2StrangeEuler(obj2CamVec);

                obj2CamSEuler.addX(totalMoveInfo.m_view.x);
                obj2CamSEuler.addY(-totalMoveInfo.m_view.y);

                obj2CamSEuler.clampY(glm::radians(-MAX_Y_DEGREE), glm::radians(MAX_Y_DEGREE));
                const auto rotatedVec = dal::strangeEuler2Vec(obj2CamSEuler);
                camera.m_pos = cpntTrans.m_pos + k_modelCamOffset + rotatedVec * len;
            }

            {
                constexpr float OBJ_CAM_DISTANCE = 2.0f;

                const auto cam2ObjVec = cpntTrans.m_pos - camera.m_pos + k_modelCamOffset;
                const auto cam2ObjSEuler = dal::vec2StrangeEuler(cam2ObjVec);
                camera.setViewPlane(cam2ObjSEuler.getX(), cam2ObjSEuler.getY());

                camera.m_pos = cpntTrans.m_pos + k_modelCamOffset - dal::resizeOnlyXZ(cam2ObjVec, OBJ_CAM_DISTANCE);
            }

            cpntTrans.updateMat();
        }

        camera.updateViewMat();
    }
    */

}


// Widgets
namespace dal {

    InputApplier::PlayerControlWidget::MoveDPad::MoveDPad(dal::Widget2* const parent, const float winWidth, const float winHeight)
        : Widget2(parent)
        , m_fixedCenterPoint(this, 1.0f, 1.0f, 1.0f, 1.0f)
        , m_touchedPoint(this, 1.0f, 1.0f, 1.0f, 1.0f)
    {
        this->onParentResize(winWidth, winHeight);

        this->m_fixedCenterPoint.setSize(RENDERED_POINT_EDGE_LEN_HALF * 2.0f, RENDERED_POINT_EDGE_LEN_HALF * 2.0f);
        this->m_fixedCenterPoint.setPos(this->makeFixedCenterPos() - RENDERED_POINT_EDGE_LEN_HALF);

        this->m_touchedPoint.setSize(RENDERED_POINT_EDGE_LEN_HALF * 2.0f, RENDERED_POINT_EDGE_LEN_HALF * 2.0f);
    }

    void InputApplier::PlayerControlWidget::MoveDPad::render(const UnilocOverlay& uniloc, const float width, const float height) {
        this->m_fixedCenterPoint.render(uniloc, width, height);

        if ( -1 != this->m_owning ) {
            this->m_touchedPoint.setPos(this->m_touchedPos - RENDERED_POINT_EDGE_LEN_HALF);
            this->m_touchedPoint.render(uniloc, width, height);
        }
    }

    InputCtrlFlag InputApplier::PlayerControlWidget::MoveDPad::onTouch(const dal::TouchEvent& e) {
        if ( this->isActive() ) {
            if ( e.m_id == this->m_owning ) {
                if ( e.m_actionType == TouchActionType::move ) {
                    this->updateTouchedPos(e.m_pos.x, e.m_pos.y, this->m_touchedPos);
                    return InputCtrlFlag::owned;
                }
                else if ( TouchActionType::up == e.m_actionType ) {
                    this->m_owning = -1;
                    return InputCtrlFlag::consumed;
                }
                else {
                    // Else case is when event type is down, which shouldn't happen.
                    // But it's really Android system who is responsible for that so I can't assure.
                    dalWarn("Got touch event type down for the id which was already being processed as down.");
                    return InputCtrlFlag::ignored;
                }
            }
            // If else ignores.
        }
        else {  // If not active.
            if ( TouchActionType::down == e.m_actionType && this->isInsideCircle(e.m_pos.x, e.m_pos.y) ) {
                // Start owning the touch id.
                this->m_owning = e.m_id;
                this->updateTouchedPos(e.m_pos.x, e.m_pos.y, this->m_touchedPos);
                return InputCtrlFlag::owned;
            }
            // If else ignores.
        }

        return InputCtrlFlag::ignored;
    }

    void InputApplier::PlayerControlWidget::MoveDPad::onParentResize(const float width, const float height) {
        const auto shorter = width < height ? width : height;
        const auto edgeLen = shorter * 0.5f;
        this->setPos(CORNER_MARGIN, height - edgeLen - CORNER_MARGIN);
        this->setSize(edgeLen, edgeLen);

        this->m_fixedCenterPoint.setPos(this->makeFixedCenterPos() - RENDERED_POINT_EDGE_LEN_HALF);
    }

    glm::vec2 InputApplier::PlayerControlWidget::MoveDPad::getRel(void) const {
        if ( !this->isActive() ) {
            return glm::vec2{ 0.0f };
        }

        dalAssert(this->getWidth() == this->getHeight());

        const auto fixedCenter = this->makeFixedCenterPos();
        return (this->m_touchedPos - fixedCenter) / (this->getWidth() * 0.5f);
    }

    bool InputApplier::PlayerControlWidget::MoveDPad::isActive(void) const {
        return -1 != this->m_owning;
    }

    // Private

    void InputApplier::PlayerControlWidget::MoveDPad::updateTouchedPos(const float x, const float y, glm::vec2& target) const {
        dalAssert(this->getWidth() == this->getHeight());

        const auto fixedCenter = this->makeFixedCenterPos();;
        target = clampVec(glm::vec2{ x, y } -fixedCenter, this->getWidth() * 0.5f) + fixedCenter;
    }

    glm::vec2 InputApplier::PlayerControlWidget::MoveDPad::makeFixedCenterPos(void) const {
        return glm::vec2{
            this->getPosX() + this->getWidth() * 0.5f,
            this->getPosY() + this->getHeight() * 0.5f
        };
    }

    bool InputApplier::PlayerControlWidget::MoveDPad::isInsideCircle(const glm::vec2& v) const {
        const float radiusSqr = this->getWidth() * this->getWidth() * 0.5f * 0.5f;
        const auto center = this->makeFixedCenterPos();
        const auto rel = v - center;
        const auto lenSqr = glm::dot(rel, rel);
        return lenSqr < radiusSqr;
    }

}


namespace dal {

    InputApplier::PlayerControlWidget::PlayerControlWidget(const float winWidth, const float winHeight)
        : Widget2(nullptr)
        , m_dpad(this, winWidth, winHeight)
        , m_owningForView(-1)
    {
        this->setPos(0.0f, 0.0f);
        this->setSize(winWidth, winHeight);
    }

    void InputApplier::PlayerControlWidget::render(const UnilocOverlay& uniloc, const float width, const float height) {
        this->m_dpad.render(uniloc, width, height);
    }

    InputCtrlFlag InputApplier::PlayerControlWidget::onTouch(const TouchEvent& e) {
        const auto dpadReturnFlag = this->m_dpad.onTouch(e);

        if ( InputCtrlFlag::ignored != dpadReturnFlag ) {
            return dpadReturnFlag;
        }
        else {
            if ( -1 != this->m_owningForView ) {
                if ( e.m_id == this->m_owningForView ) {
                    if ( e.m_actionType == TouchActionType::move ) {
                        this->m_viewTouchAccum = e.m_pos - this->m_lastViewTouchPos;
                        this->m_lastViewTouchPos = e.m_pos;
                        return InputCtrlFlag::owned;
                    }
                    else if ( TouchActionType::up == e.m_actionType ) {
                        this->m_owningForView = -1;
                        return InputCtrlFlag::consumed;
                    }
                    else {
                        // Else case is when event type is down, which shouldn't happen.
                        // But it's really Android system who is responsible for that so I can't assure.
                        dalWarn("Got touch event type down for the id which was already being processed as down.");
                        return InputCtrlFlag::ignored;
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
                    return InputCtrlFlag::owned;
                }
                // If else ignores.
            }

            return InputCtrlFlag::ignored;
        }
    }

    InputCtrlFlag InputApplier::PlayerControlWidget::onKeyInput(const KeyboardEvent& e, const KeyStatesRegistry& keyStates) {
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

        return InputCtrlFlag::consumed;
    }

    void InputApplier::PlayerControlWidget::onParentResize(const float width, const float height) {
        this->m_dpad.onParentResize(width, height);

        this->setPos(0.0f, 0.0f);
        this->setSize(width, height);
    }

    void InputApplier::PlayerControlWidget::onFocusChange(const bool v) {
        if ( v ) {
            this->m_keyboardMoveInfo.clear();
        }
    }

    MoveInputInfo InputApplier::PlayerControlWidget::getMoveInfo(const float deltaTime, const float winWidth, const float winHeight) {
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

    // Private

    glm::vec2 InputApplier::PlayerControlWidget::getMoveVec(void) const {
        return this->m_dpad.getRel();
    }

    glm::vec2 InputApplier::PlayerControlWidget::getResetViewAccum(void) {
        const auto tmp = this->m_viewTouchAccum;
        this->m_viewTouchAccum = glm::vec2{ 0.0f };
        return tmp;
    }

}


namespace dal {

    InputApplier::InputApplier(OverlayMaster& overlayMas, const unsigned int width, const unsigned int height)
        : mFSM(GlobalGameState::game)
        , m_overlayMas(overlayMas)
        , m_ctrlInputWidget(static_cast<float>(width), static_cast<float>(height))
    {
        this->mHandlerName = "InputApplier";
        EventGod::getinst().registerHandler(this, EventType::global_fsm_change);

        this->m_overlayMas.giveBackgroudWidgetRef(&this->m_ctrlInputWidget);
    }

    InputApplier::~InputApplier(void) {
        EventGod::getinst().deregisterHandler(this, EventType::global_fsm_change);

        this->m_overlayMas.removeWidgetRef(&this->m_ctrlInputWidget);
    }

    void InputApplier::onEvent(const EventStatic& e) {
        switch ( e.type ) {

        case EventType::global_fsm_change:
            this->mFSM = GlobalGameState(e.intArg1);
            break;
        default:
            dalWarn("InputApplier can't handle this event: {}"_format(getEventTypeStr(e.type)));
            break;

        }
    }

    /*
    void InputApplier::apply(const float deltaTime, StrangeEulerCamera& camera, const entt::entity targetEntity, entt::registry& reg) {
        const float winWidth = (float)dal::GlobalStateGod::getinst().getWinWidth();
        const float winHeight = (float)dal::GlobalStateGod::getinst().getWinHeight();

        const auto info = this->m_ctrlInputWidget.getMoveInfo(deltaTime, winWidth, winHeight);

        apply_topdown(deltaTime, info, camera, targetEntity, reg);
        //apply_flyForPlatform(deltaTime, info, camera);
    }
    */

    void InputApplier::apply(const float deltaTime, StrangeEulerCamera& camera, cpnt::CharacterState& state) {
        const auto winSize = GlobalStateGod::getinst().getWinSizeFloat();
        const auto info = this->m_ctrlInputWidget.getMoveInfo(deltaTime, winSize.x, winSize.y);
        state.update(deltaTime, info);
    }

}  // namespace dal