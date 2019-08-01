#include "c_input_apply.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"


using namespace fmt::literals;


namespace {

    const float DPAD_RADIUS_FACTOR_INV = 7.0f;

}


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

    /*
    In OpenGL coordinate system, if input is (x, z), rotation follows left hand rule.
    */
    glm::vec2 rotateVec2(const glm::vec2& v, const float radians) {
        const auto sinVal = sin(radians);
        const auto cosVal = cos(radians);

        return glm::vec2{
            v.x * cosVal - v.y * sinVal,
            v.x * sinVal + v.y * cosVal
        };
    }

    /*
    Normalize xz components on the plane (0, 1, 0, -v.y).
    So if v.y is longer than length, all 3 components must be resized to make it onto sphere.
    */
    glm::vec3 resizeOnlyXZ(const glm::vec3& v, const float sphereRadius) {
        const auto circleOfIntersecRadiusSqr = sphereRadius * sphereRadius - v.y * v.y;
        if ( circleOfIntersecRadiusSqr > 0.0f ) {
            const auto circleOfIntersecRadius = sqrt(circleOfIntersecRadiusSqr);
            const auto resizedVecXZ = glm::normalize(glm::vec2{ v.x, v.z }) * circleOfIntersecRadius;
            return glm::vec3{ resizedVecXZ.x, v.y, resizedVecXZ.y };
        }
        else {
            return glm::normalize(v) * sphereRadius;
        }
    }

    glm::vec2 clampVec(glm::vec2 v, const float maxLen) {
        const auto lenSqr = glm::dot(v, v);
        if ( lenSqr > maxLen * maxLen ) {
            v *= glm::inversesqrt(lenSqr) * maxLen;
        }
        return v;
    }

    template <unsigned int MAX_LEN>
    glm::vec2 clampVec(glm::vec2 v) {
        constexpr auto maxLen = static_cast<float>(MAX_LEN);
        constexpr auto maxLenSqr = static_cast<float>(MAX_LEN * MAX_LEN);

        const auto lenSqr = glm::dot(v, v);
        if ( lenSqr > maxLenSqr ) {
            v *= glm::inversesqrt(lenSqr) * maxLen;
        }
        return v;
    }

}  // namespace


namespace {

    struct NoclipMoveInfo {
        float xView = 0.0f, yView = 0.0f;
        float xMovePlane = 0.0f, zMovePlane = 0.0f;
        float vertical = 0.0f;

        NoclipMoveInfo& operator+=(const NoclipMoveInfo& other) {
            this->xView += other.xView;
            this->yView += other.yView;
            this->xMovePlane += other.xMovePlane;
            this->zMovePlane += other.zMovePlane;
            this->vertical += other.vertical;

            return *this;
        }
    };

}  // namespace


namespace {

    /*
    void apply_flyDirectional(const float deltaTime, dal::StrangeEulerCamera& camera, dal::OverlayMaster& overlay) {
        glm::vec2 totalMovePlane{ 0.0 };  // This means it must represent move direction when targetViewDir == { 0, 0 }.
        float moveUpOrDown = 0.0f;

        g_keyboardMas.fetch();
        g_touchMas.fetch();

        // Take inputs
        {
            NoclipMoveInfo keyboardInfo;
            if ( g_keyboardMas.makeMoveInfo(keyboardInfo, deltaTime) ) {
                totalMovePlane += glm::vec2{ keyboardInfo.xMovePlane, keyboardInfo.zMovePlane };
                camera.addViewPlane(keyboardInfo.xView, keyboardInfo.yView);
                moveUpOrDown += keyboardInfo.vertical;
            }

            NoclipMoveInfo touchInfo;
            if ( g_touchMas.makeMoveInfo(touchInfo, overlay) ) {
                totalMovePlane += glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
                camera.addViewPlane(touchInfo.xView, touchInfo.yView);
                moveUpOrDown += touchInfo.vertical;
            }

            camera.updateViewMat();
        }

        // Return if target doesn't need to move.
        if ( totalMovePlane.x == 0.0f && totalMovePlane.y == 0.0f && moveUpOrDown == 0.0f ) return;

        // Apply move direction
        {
            glm::mat4 viewMat{ 1.0 };
            const auto viewVec = camera.getViewPlane();
            viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
            viewMat = glm::rotate(viewMat, viewVec.y, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::vec3 moveDirection{ viewMat * glm::vec4{totalMovePlane.x, 0.0, totalMovePlane.y, 1.0} };
            if ( moveUpOrDown ) moveDirection.y = moveUpOrDown;

            //moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
            const float moveMultiplier = 5.0f * deltaTime;
            camera.m_pos += moveDirection * moveMultiplier;
        }

        camera.updateViewMat();
    }

    void apply_flyPlane(const float deltaTime, dal::StrangeEulerCamera& camera, dal::OverlayMaster& overlay) {
        glm::vec2 totalMovePlane{ 0.0 };  // This means it must represent move direction when targetViewDir == { 0, 0 }.
        float moveUpOrDown = 0.0f;

        g_keyboardMas.fetch();
        g_touchMas.fetch();

        // Take inputs
        {
            NoclipMoveInfo keyboardInfo;
            if ( g_keyboardMas.makeMoveInfo(keyboardInfo, deltaTime) ) {
                totalMovePlane += glm::vec2{ keyboardInfo.xMovePlane, keyboardInfo.zMovePlane };
                camera.addViewPlane(keyboardInfo.xView, keyboardInfo.yView);
                moveUpOrDown += keyboardInfo.vertical;
            }

            NoclipMoveInfo touchInfo;
            if ( g_touchMas.makeMoveInfo(touchInfo, overlay) ) {
                totalMovePlane += glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
                camera.addViewPlane(touchInfo.xView, touchInfo.yView);
                moveUpOrDown += touchInfo.vertical;
            }

            camera.updateViewMat();
        }

        // Return if target doesn't need to move.
        if ( totalMovePlane.x == 0.0f && totalMovePlane.y == 0.0f && moveUpOrDown == 0.0f ) return;

        // Apply move direction
        {
            glm::mat4 viewMat{ 1.0 };
            const auto viewVec = camera.getViewPlane();
            viewMat = glm::rotate(viewMat, -viewVec.x, glm::vec3(0.0f, 1.0f, 0.0f));
            //viewMat = glm::rotate(viewMat, targetViewDir->y, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::vec3 moveDirection{ viewMat * glm::vec4{totalMovePlane.x, 0.0, totalMovePlane.y, 1.0} };
            if ( moveUpOrDown ) moveDirection.y = moveUpOrDown;

            //moveDirection = glm::normalize(moveDirection);  // Maybe this is redundant.
            const float moveMultiplier = 5.0f * deltaTime;
            camera.m_pos += moveDirection * moveMultiplier;
            camera.m_pos.y += moveDirection.y * 50.0f * deltaTime;
        }

        camera.updateViewMat();
    }

    void apply_topdown(const float deltaTime, dal::StrangeEulerCamera& camera, dal::OverlayMaster& overlay,
        const entt::entity targetEntity, entt::registry& reg) {
        // This means it must represent move direction when targetViewDir == { 0, 0 }
        NoclipMoveInfo totalMoveInfo{ 0.0f };

        if ( reg.has<dal::cpnt::AnimatedModel>(targetEntity) ) {
            reg.get<dal::cpnt::AnimatedModel>(targetEntity).m_animState.setSelectedAnimeIndex(0);
        }

        // Take inputs
        {
            g_keyboardMas.fetch();
            g_touchMas.fetch();

            NoclipMoveInfo keyboardInfo;
            const auto keyUpdated = g_keyboardMas.makeMoveInfo(keyboardInfo, deltaTime);
            if ( keyUpdated ) {
                totalMoveInfo += keyboardInfo;
            }

            NoclipMoveInfo touchInfo;
            const auto touchUpdated = g_touchMas.makeMoveInfo(touchInfo, overlay);
            if ( touchUpdated ) {
                //const auto moveVec = clampVec<1>(glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane });
                const auto moveVec = glm::vec2{ touchInfo.xMovePlane, touchInfo.zMovePlane };
                touchInfo.xMovePlane = moveVec.x;
                touchInfo.zMovePlane = moveVec.y;

                totalMoveInfo += touchInfo;
            }

            if ( !keyUpdated && !touchUpdated ) {
                return;
            }
        }

        // Post process
        {
            const auto clampedMovePlane = clampVec<1>(glm::vec2{ totalMoveInfo.xMovePlane, totalMoveInfo.zMovePlane });
            totalMoveInfo.xMovePlane = clampedMovePlane.x;
            totalMoveInfo.zMovePlane = clampedMovePlane.y;
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
                const auto rotatedMoveVec = rotateVec2(glm::vec2{ totalMoveInfo.xMovePlane, totalMoveInfo.zMovePlane }, camera.getStrangeEuler().getX());

                const auto deltaPos = glm::vec3{ rotatedMoveVec.x, totalMoveInfo.vertical, rotatedMoveVec.y } *deltaTime * 5.0f;
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

                obj2CamSEuler.addX(totalMoveInfo.xView);
                obj2CamSEuler.addY(-totalMoveInfo.yView);

                obj2CamSEuler.clampY(glm::radians(-MAX_Y_DEGREE), glm::radians(MAX_Y_DEGREE));
                const auto rotatedVec = dal::strangeEuler2Vec(obj2CamSEuler);
                camera.m_pos = cpntTrans.m_pos + k_modelCamOffset + rotatedVec * len;
            }

            {
                constexpr float OBJ_CAM_DISTANCE = 2.0f;

                const auto cam2ObjVec = cpntTrans.m_pos - camera.m_pos + k_modelCamOffset;
                const auto cam2ObjSEuler = dal::vec2StrangeEuler(cam2ObjVec);
                camera.setViewPlane(cam2ObjSEuler.getX(), cam2ObjSEuler.getY());

                camera.m_pos = cpntTrans.m_pos + k_modelCamOffset - resizeOnlyXZ(cam2ObjVec, OBJ_CAM_DISTANCE);
            }

            camera.updateViewMat();
            cpntTrans.updateMat();
        }
    }

    void apply_menuControl(dal::OverlayMaster& overlay) {
        g_touchMas.fetch();
        g_keyboardMas.fetch();

        for ( const auto& com : g_keyboardMas.getCommands() ) {
            switch ( com.getType() ) {

            case KeyboardStatesMaster::KeyboardCommand::CmdType::text:
                overlay.onKeyInput(com.getData_text());
                break;
            case KeyboardStatesMaster::KeyboardCommand::CmdType::toggle_game_state:
                toggleGameState();
                break;
            default:
                dalWarn("Unhandled keyboard command: "s + std::to_string(int(com.getType())));

            }
        }

        glm::vec2 pos;
        for ( const auto& cmd : g_touchMas.getCommands() ) {
            switch ( cmd.getType() ) {

            case TouchStatesMaster::TouchCommand::CmdType::click:
                pos = cmd.getClickPos();
                overlay.onClick(pos.x, pos.y);
                break;
            default:
                dalWarn("Unhandled touch command: "s + std::to_string(int(cmd.getType())));

            }
        }
    }
    */

}  // namespace


namespace {

    void apply_topdown(const float deltaTime, const dal::InputApplier::MoveInputInfo& totalMoveInfo,
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
                const auto rotatedMoveVec = rotateVec2(glm::vec2{ totalMoveInfo.m_move.x, totalMoveInfo.m_move.y }, camera.getStrangeEuler().getX());

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

                camera.m_pos = cpntTrans.m_pos + k_modelCamOffset - resizeOnlyXZ(cam2ObjVec, OBJ_CAM_DISTANCE);
            }

            camera.updateViewMat();
            cpntTrans.updateMat();
        }
    }

}


namespace {

    static constexpr float CORNER_MARGIN = 40.0f;
    static constexpr float RENDERED_POINT_EDGE_LEN_HALF = 10.0f;

}


// Widgets
namespace dal {

    InputApplier::MoveDPad::MoveDPad(dal::Widget2* const parent, const float winWidth, const float winHeight)
        : Widget2(parent)
        , m_fixedCenterPoint(this, 1.0f, 1.0f, 1.0f, 1.0f)
        , m_touchedPoint(this, 1.0f, 1.0f, 1.0f, 1.0f)
    {
        this->onParentResize(winWidth, winHeight);

        this->m_fixedCenterPoint.setSize(RENDERED_POINT_EDGE_LEN_HALF * 2.0f, RENDERED_POINT_EDGE_LEN_HALF * 2.0f);
        this->m_fixedCenterPoint.setPos(this->makeFixedCenterPos() - RENDERED_POINT_EDGE_LEN_HALF);

        this->m_touchedPoint.setSize(RENDERED_POINT_EDGE_LEN_HALF * 2.0f, RENDERED_POINT_EDGE_LEN_HALF * 2.0f);
    }

    void InputApplier::MoveDPad::render(const UnilocOverlay& uniloc, const float width, const float height) {
        this->m_fixedCenterPoint.render(uniloc, width, height);

        if ( -1 != this->m_owning ) {
            this->m_touchedPoint.setPos(this->m_touchedPos - RENDERED_POINT_EDGE_LEN_HALF);
            this->m_touchedPoint.render(uniloc, width, height);
        }
    }

    InputCtrlFlag InputApplier::MoveDPad::onTouch(const dal::TouchEvent& e) {
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

    void InputApplier::MoveDPad::onParentResize(const float width, const float height) {
        const auto shorter = width < height ? width : height;
        const auto edgeLen = shorter * 0.5f;
        this->setPos(CORNER_MARGIN, height - edgeLen - CORNER_MARGIN);
        this->setSize(edgeLen, edgeLen);

        this->m_fixedCenterPoint.setPos(this->makeFixedCenterPos() - RENDERED_POINT_EDGE_LEN_HALF);
    }

    glm::vec2 InputApplier::MoveDPad::getRel(void) const {
        if ( !this->isActive() ) {
            return glm::vec2{ 0.0f };
        }

        dalAssert(this->getWidth() == this->getHeight());

        const auto fixedCenter = this->makeFixedCenterPos();
        return (this->m_touchedPos - fixedCenter) / (this->getWidth() * 0.5f);
    }

    bool InputApplier::MoveDPad::isActive(void) const {
        return -1 != this->m_owning;
    }

    // Private

    void InputApplier::MoveDPad::updateTouchedPos(const float x, const float y, glm::vec2& target) const {
        dalAssert(this->getWidth() == this->getHeight());

        const auto fixedCenter = this->makeFixedCenterPos();;
        target = clampVec(glm::vec2{ x, y } -fixedCenter, this->getWidth() * 0.5f) + fixedCenter;
    }

    glm::vec2 InputApplier::MoveDPad::makeFixedCenterPos(void) const {
        return glm::vec2{
            this->getPosX() + this->getWidth() * 0.5f,
            this->getPosY() + this->getHeight() * 0.5f
        };
    }

    bool InputApplier::MoveDPad::isInsideCircle(const glm::vec2& v) const {
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
            this->m_keyboardMoveInfo.m_vertical += 0.5f;
        }
        if ( keyStates[(dal::KeySpec::lshfit)].m_pressed ) {
            this->m_keyboardMoveInfo.m_vertical -= 0.5f;
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

    InputApplier::MoveInputInfo InputApplier::PlayerControlWidget::getMoveInfo(const float deltaTime, const float winWidth, const float winHeight) {
        MoveInputInfo info;

        const float widthOrHeightButShorter = winWidth < winHeight ? winWidth : winHeight;
        const float viewMultiplier = 5.0f / widthOrHeightButShorter;

        {
            info += this->m_keyboardMoveInfo;
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
        if ( GlobalGameState::game == this->mFSM ) {
#if defined(_WIN32)
            apply_topdown(deltaTime, camera, this->m_overlayMas, targetEntity, reg);
#else defined(__ANDROID__)
            apply_topdown(deltaTime, camera, this->m_overlayMas, targetEntity, reg);
#endif
        }
        else if ( GlobalGameState::menu == this->mFSM ) {
            apply_menuControl(this->m_overlayMas);
        }
        else {
            dalAbort("Unknown global game state.");
        }
    }
    */

    void InputApplier::apply(const float deltaTime, StrangeEulerCamera& camera, const entt::entity targetEntity, entt::registry& reg) {
        const float winWidth = (float)dal::GlobalStateGod::getinst().getWinWidth();
        const float winHeight = (float)dal::GlobalStateGod::getinst().getWinHeight();

        const auto info = this->m_ctrlInputWidget.getMoveInfo(deltaTime, winWidth, winHeight);

        apply_topdown(deltaTime, info, camera, targetEntity, reg);
    }

}  // namespace dal