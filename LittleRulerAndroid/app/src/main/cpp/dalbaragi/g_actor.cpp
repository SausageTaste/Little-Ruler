#include "g_actor.h"

#include <memory>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"


using namespace fmt::literals;


// Util functions
namespace {

    inline constexpr float clampStrangeEulerX(const float x) {
        constexpr auto plus360Deg = glm::radians(360.0f);

        if ( x >= plus360Deg ) {
            return fmod(x, plus360Deg);
        }
        else if ( x < glm::radians(0.0f) ) {
            return x + glm::trunc(x / plus360Deg) * plus360Deg + plus360Deg;
        }
        else {
            return x;
        }
    }

    inline constexpr float clampStrangeEulerY(const float y) {
        constexpr auto plus90Degree = glm::radians(89.9f);
        constexpr auto minus90Degree = glm::radians(-89.9f);

        if ( y > plus90Degree ) {
            return plus90Degree;
        }
        else if ( y < minus90Degree ) {
            return minus90Degree;
        }
        else {
            return y;
        }
    }

    float getSignedDistance_point2Plane(const glm::vec3& v, const glm::vec4& p) {
        return abs(p.x*v.x + p.y*v.y + p.z*v.z + p.w) * glm::inversesqrt(p.x*p.x + p.y*p.y + p.z+p.z);
    }

    void applybindingCameraToModel(dal::StrangeEulerCamera& camera, const float deltaTime, const dal::MoveInputInfo& totalMoveInfo,
        const glm::vec3 mdlThisPos, const glm::vec3 mdlLastPos)
    {
        // Apply move direction
        {
            const glm::vec3 MODEL_ORIGIN_OFFSET{ 0.0f, 1.3f, 0.0f };
            constexpr float MAX_Y_DEGREE = 75.0f;
            constexpr float CAM_ROTATE_SPEED_INV = 0.7f;
            static_assert(0.0f <= CAM_ROTATE_SPEED_INV && CAM_ROTATE_SPEED_INV <= 1.0f);

            const auto camOrigin = mdlThisPos + MODEL_ORIGIN_OFFSET;

            {
                const auto deltaPos = mdlThisPos - mdlLastPos;
                camera.m_pos += deltaPos * CAM_ROTATE_SPEED_INV;
            }

            {
                const auto obj2CamVec = camera.m_pos - camOrigin;
                const auto len = glm::length(obj2CamVec);
                auto obj2CamSEuler = dal::vec2StrangeEuler(obj2CamVec);

                obj2CamSEuler.addX(totalMoveInfo.m_view.x);
                obj2CamSEuler.addY(-totalMoveInfo.m_view.y);
                //obj2CamSEuler.clampY(glm::radians(50.0f), glm::radians(80.0f));

                obj2CamSEuler.clampY(glm::radians(-MAX_Y_DEGREE), glm::radians(MAX_Y_DEGREE));
                const auto rotatedVec = dal::strangeEuler2Vec(obj2CamSEuler);
                camera.m_pos = camOrigin + rotatedVec * len;
            }

            {
                constexpr float OBJ_CAM_DISTANCE = 3.0f;

                const auto cam2ObjVec = camOrigin - camera.m_pos;
                const auto cam2ObjSEuler = dal::vec2StrangeEuler(cam2ObjVec);
                camera.setViewPlane(cam2ObjSEuler.getX(), cam2ObjSEuler.getY());

                camera.m_pos = camOrigin - dal::resizeOnlyXZ(cam2ObjVec, OBJ_CAM_DISTANCE);
            }
        }

        camera.updateViewMat();
    }

    void applyMove(dal::cpnt::Transform& cpntTrans, dal::cpnt::AnimatedModel& animModel, const dal::StrangeEulerCamera camera,
        const float deltaTime, const dal::MoveInputInfo& totalMoveInfo)
    {
        constexpr float CAM_ROTATE_SPEED_INV = 1.0f;
        static_assert(0.0f <= CAM_ROTATE_SPEED_INV && CAM_ROTATE_SPEED_INV <= 1.0f);

        const auto camViewVec = dal::strangeEuler2Vec(camera.getStrangeEuler());
        const auto rotatorAsCamX = glm::rotate(glm::mat4{ 1.0f }, camera.getStrangeEuler().getX(), glm::vec3{ 0.0f, 1.0f, 0.0f });
        const auto rotatedMoveVec = dal::rotateVec2(glm::vec2{ totalMoveInfo.m_move.x, totalMoveInfo.m_move.y }, camera.getStrangeEuler().getX());

        const auto deltaPos = glm::vec3{ rotatedMoveVec.x, 0.0f, rotatedMoveVec.y } *deltaTime * 5.0f;
        cpntTrans.m_pos += deltaPos;
        if ( rotatedMoveVec.x != 0.0f || rotatedMoveVec.y != 0.0f ) {  // If moved position
            cpntTrans.m_quat = dal::rotateQuat(glm::quat{}, atan2(rotatedMoveVec.x, rotatedMoveVec.y), glm::vec3{ 0.0f, 1.0f, 0.0f });

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

            cpntTrans.updateMat();
        }
    }

}


// Head functions
namespace dal {

    glm::quat rotateQuat(const glm::quat& q, const float radians, const glm::vec3& selector) {
        return glm::normalize(glm::angleAxis(radians, selector) * q);
    }

    glm::vec2 rotateVec2(const glm::vec2& v, const float radians) {
        const auto sinVal = sin(radians);
        const auto cosVal = cos(radians);

        return glm::vec2{
            v.x * cosVal - v.y * sinVal,
            v.x * sinVal + v.y * cosVal
        };
    }

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


    glm::vec3 strangeEuler2Vec(const StrangeEuler& se) {
        const auto x = se.getX();
        const auto y = se.getY();

        return glm::vec3{
            sin(x) * cos(y),
            sin(y),
            -cos(x) * cos(y)
        };
    }

    StrangeEuler vec2StrangeEuler(glm::vec3 v) {
        const glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        v = glm::normalize(v);

        float y = glm::radians(90.0f) - acos(glm::dot(up, v));

        float x = -atan2(-v.z, v.x) + glm::radians(450.0f);
        x = fmod(x, glm::radians(360.0f));

        return StrangeEuler{ x, y };
    }

}


// StrangeEuler
namespace dal {

    StrangeEuler::StrangeEuler(void)
        : x(0.0f), y(0.0f)
    {

    }

    StrangeEuler::StrangeEuler(const float x, const float y)
        : x(x), y(y)
    {

    }

    float StrangeEuler::getX(void) const {
        return this->x;
    }

    float StrangeEuler::getY(void) const {
        return this->y;
    }

    void StrangeEuler::setX(const float v) {
        this->x = clampStrangeEulerX(v);
    }

    void StrangeEuler::setY(const float v) {
        this->y = clampStrangeEulerY(v);
    }

    void StrangeEuler::addX(const float v) {
        this->x = clampStrangeEulerX(this->x + v);
    }

    void StrangeEuler::addY(const float v) {
        this->y = clampStrangeEulerY(this->y + v);
    }

    void StrangeEuler::clampY(const float min, const float max) {
        if ( y > max ) {
            this->y = max;
        }
        else if ( y < min ) {
            this->y = min;
        }
    }

    glm::mat4 StrangeEuler::makeRotateMat(void) const {
        glm::mat4 mat{ 1.0f };
        mat = glm::rotate(mat, this->y, glm::vec3(-1.0f, 0.0f, 0.0f));
        mat = glm::rotate(mat, this->x, glm::vec3(0.0f, 1.0f, 0.0f));
        return mat;
    }

}


// StrangeEulerCamera
namespace dal {

    void StrangeEulerCamera::updateViewMat(void) {
        const auto rot = this->m_stranEuler.makeRotateMat();
        this->m_viewMat = glm::translate(rot, glm::vec3(-this->m_pos.x, -this->m_pos.y, -this->m_pos.z));
    }

    std::pair<glm::vec3, glm::mat4> StrangeEulerCamera::makeReflected(const float planeHeight) const {
        dal::StrangeEulerCamera newCam = *this;

        newCam.m_pos.y = 2.0f * planeHeight - newCam.m_pos.y;

        const auto camViewPlane = newCam.getViewPlane();
        newCam.setViewPlane(camViewPlane.x, -camViewPlane.y);

        newCam.updateViewMat();
        
        return std::make_pair(newCam.m_pos, newCam.getViewMat());
    }


    glm::vec2 StrangeEulerCamera::getViewPlane(void) const {
        return glm::vec2{ this->m_stranEuler.getX(), this->m_stranEuler.getY() };
    }

    StrangeEuler& StrangeEulerCamera::getStrangeEuler(void) {
        return this->m_stranEuler;
    }

    const StrangeEuler& StrangeEulerCamera::getStrangeEuler(void) const {
        return this->m_stranEuler;
    }

    void StrangeEulerCamera::setViewPlane(const float x, const float y) {
        this->m_stranEuler.setX(x);
        this->m_stranEuler.setY(y);
    }


    void StrangeEulerCamera::addViewPlane(const float x, const float y) {
        this->m_stranEuler.addX(x);
        this->m_stranEuler.addY(y);
    }

}


// ActorInfo
namespace dal {

    ActorInfo::ActorInfo(const std::string& actorName, const bool flagStatic)
        : m_name(actorName),
        m_static(flagStatic)
    {

    }

    const glm::mat4& ActorInfo::getModelMat(void) {
        if ( this->m_matNeedUpdate ) {
            const auto identity = glm::mat4{ 1.0f };
            const auto scaleMat = glm::scale(identity, glm::vec3{ this->m_scale, this->m_scale , this->m_scale });
            const auto translateMat = glm::translate(identity, this->m_pos);
            this->m_modelMat = translateMat * glm::mat4_cast(this->m_quat) * scaleMat;
            this->m_matNeedUpdate = false;
        }

        return this->m_modelMat;
    }


    void ActorInfo::setQuat(const glm::quat& q) {
        this->m_matNeedUpdate = true;
        this->m_quat = q;
    }

    void ActorInfo::rotate(const float v, const glm::vec3& selector) {
        this->m_matNeedUpdate = true;
        this->m_quat = glm::normalize(glm::angleAxis(v, selector) * this->m_quat);
    }


    const glm::vec3& ActorInfo::getPos(void) const {
        return this->m_pos;
    }

    void ActorInfo::setPos(const glm::vec3& v) {
        this->m_matNeedUpdate = true;
        this->m_pos = v;
    }

    void ActorInfo::addPos(const glm::vec3& v) {
        this->m_matNeedUpdate = true;
        this->m_pos += v;
    }

    float ActorInfo::getScale(void) const {
        return this->m_scale;
    }

    void ActorInfo::setScale(const float v) {
        this->m_scale = v;
    }

}


namespace dal::cpnt {

    Transform::Transform(void) {
        this->updateMat();
    }

    void Transform::updateMat(void) {
        const auto identity = glm::mat4{ 1.0f };
        const auto scaleMat = glm::scale(identity, glm::vec3{ this->m_scale, this->m_scale , this->m_scale });
        const auto translateMat = glm::translate(identity, this->m_pos);
        this->m_modelMat = translateMat * glm::mat4_cast(this->m_quat) * scaleMat;
    }

}


// MoveInputInfo and ICharaState
namespace dal {

    void MoveInputInfo::merge(const MoveInputInfo& other) {
        this->m_view += other.m_view;
        this->m_move += other.m_move;
        this->m_jump |= other.m_jump;
    }

    void MoveInputInfo::clear(void) {
        this->m_view = glm::vec2{ 0.0f };
        this->m_move = glm::vec2{ 0.0f };
        this->m_jump = false;
    }

    bool MoveInputInfo::hasMovement(void) const {
        if ( 0.0f != this->m_move.x ) {
            return true;
        }
        else if ( 0.0f != this->m_move.y ) {
            return true;
        }
        else {
            return false;
        }
    }

    bool MoveInputInfo::hasViewMove(void) const {
        if ( 0.0f != this->m_view.x ) {
            return true;
        }
        else if ( 0.0f != this->m_view.y ) {
            return true;
        }
        else {
            return false;
        }
    }


    ICharaState::ICharaState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera)
        : m_transform(transform)
        , m_model(model)
        , m_camera(camera)
    {

    }

}


// ICharaState derived
namespace {

    class CharaIdleState : public dal::ICharaState {

    public:
        CharaIdleState(dal::cpnt::Transform& transform, dal::cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera)
            : ICharaState(transform, model, camera)
        {

        }

        virtual void enter(void) override {
            this->m_model.m_animState.setSelectedAnimeIndex(0);

            dalVerbose("IDLE");
        }

        virtual void exit(void) override {
            dalVerbose("idle");
        }

        virtual void process(const float deltaTime, const dal::MoveInputInfo& info) override {
            applybindingCameraToModel(this->m_camera, deltaTime, info, this->m_transform.m_pos, this->m_transform.m_pos);
        }

        virtual dal::ICharaState* exec(const float deltaTime, const dal::MoveInputInfo& info) override;

    };


    class CharaWalkState : public dal::ICharaState {

    public:
        CharaWalkState(dal::cpnt::Transform& transform, dal::cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera)
            : ICharaState(transform, model, camera)
        {

        }

        virtual void enter(void) override {
            this->m_model.m_animState.setSelectedAnimeIndex(1);

            dalVerbose("WALK");
        }

        virtual void exit(void) override {
            dalVerbose("walk");
        }

        virtual void process(const float deltaTime, const dal::MoveInputInfo& info) override {
            const auto mdlLastPos = this->m_transform.m_pos;
            applyMove(this->m_transform, this->m_model, this->m_camera, deltaTime, info);
            applybindingCameraToModel(this->m_camera, deltaTime, info, this->m_transform.m_pos, mdlLastPos);
        }

        virtual dal::ICharaState* exec(const float deltaTime, const dal::MoveInputInfo& info) override;

    };

}


// Chara states exec functions
namespace {

    dal::ICharaState* CharaIdleState::exec(const float deltaTime, const dal::MoveInputInfo& info) {
        if ( info.hasMovement() ) {
            std::unique_ptr<CharaIdleState> byebye{ this };
            auto newState = new CharaWalkState(this->m_transform, this->m_model, this->m_camera);

            this->exit();
            newState->enter();
            newState->process(deltaTime, info);

            return newState;
        }
        else {
            this->process(deltaTime, info);
            return this;
        }
    }

    dal::ICharaState* CharaWalkState::exec(const float deltaTime, const dal::MoveInputInfo& info) {
        if ( !info.hasMovement() ) {
            std::unique_ptr<CharaWalkState> byebye{ this };
            auto newState = new CharaIdleState(this->m_transform, this->m_model, this->m_camera);

            this->exit();
            newState->enter();
            newState->process(deltaTime, info);

            return newState;
        }
        else {
            this->process(deltaTime, info);
            return this;
        }
    }

}


namespace dal::cpnt {

    CharacterState::CharacterState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::StrangeEulerCamera& camera) 
        : m_currentState(new CharaIdleState{ transform, model, camera })
    {

    }

    void CharacterState::update(const float deltaTime, const MoveInputInfo& info) {
        this->m_currentState = this->m_currentState->exec(deltaTime, info);
    }

}