#include "g_charastate.h"

#include <fmt/format.h>

#include <d_logger.h>

#include "u_math.h"
#include "p_scene.h"


using namespace fmt::literals;


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


    ICharaState::ICharaState(dal::FPSEulerCamera& camera, SceneGraph& scene)
        : m_camera(camera)
        , m_scene(scene)
    {

    }

}


// Util functions
namespace {

    void applybindingCameraToModel(dal::FPSEulerCamera& camera, const float deltaTime, const dal::MoveInputInfo& totalMoveInfo,
        const glm::vec3 mdlThisPos, const glm::vec3 mdlLastPos)
    {
        // Apply move direction
        {
            const glm::vec3 MODEL_ORIGIN_OFFSET{ 0.f, 1.f, 0.f };
            constexpr float MAX_Y_DEGREE = 75.0f;
            constexpr float CAM_ROTATE_SPEED_INV = 1.f;
            constexpr float OBJ_CAM_DISTANCE = 3.f;

            static_assert(0.0f <= CAM_ROTATE_SPEED_INV && CAM_ROTATE_SPEED_INV <= 1.0f);

            const auto camOrigin = mdlThisPos + MODEL_ORIGIN_OFFSET;

            {
                const auto deltaPos = mdlThisPos - mdlLastPos;
                camera.m_pos += deltaPos * CAM_ROTATE_SPEED_INV;
            }

            {
                const auto obj2CamVec = camera.m_pos - camOrigin;
                const auto len = glm::length(obj2CamVec);
                auto obj2CamSEuler = dal::vec2fpsEuler(obj2CamVec);

                obj2CamSEuler.addX(totalMoveInfo.m_view.x);
                obj2CamSEuler.addY(-totalMoveInfo.m_view.y);
                //obj2CamSEuler.clampY(glm::radians(50.0f), glm::radians(80.0f));

                obj2CamSEuler.clampY(glm::radians(-MAX_Y_DEGREE), glm::radians(MAX_Y_DEGREE));
                const auto rotatedVec = dal::fpsEuler2vec(obj2CamSEuler);
                camera.m_pos = camOrigin + rotatedVec * len;
            }

            {
                // It break when OBJ_CAM_DISTANCE's value is lower than 3.

                const auto cam2ObjVec = camOrigin - camera.m_pos;
                const auto cam2ObjSEuler = dal::vec2fpsEuler(cam2ObjVec);
                camera.setViewPlane(cam2ObjSEuler.x(), cam2ObjSEuler.y());

                camera.m_pos = camOrigin - dal::resizeOnlyXZ(cam2ObjVec, OBJ_CAM_DISTANCE);
            }
        }

        camera.updateViewMat();
    }

    void applyMove(dal::cpnt::Transform& cpntTrans, dal::cpnt::AnimatedModel& animModel, const dal::FPSEulerCamera camera,
        const float deltaTime, const dal::MoveInputInfo& totalMoveInfo)
    {
        constexpr float CAM_ROTATE_SPEED_INV = 1.0f;
        static_assert(0.0f <= CAM_ROTATE_SPEED_INV && CAM_ROTATE_SPEED_INV <= 1.0f);

        // const auto camViewVec = dal::strangeEuler2Vec(camera.eulerAngles());
        // const auto rotatorAsCamX = glm::rotate(glm::mat4{ 1.0f }, camera.eulerAngles().getX(), glm::vec3{ 0.0f, 1.0f, 0.0f });
        const auto rotatedMoveVec = dal::rotateVec2(glm::vec2{ totalMoveInfo.m_move.x, totalMoveInfo.m_move.y }, camera.eulerAngles().x());

        const auto deltaPos = glm::vec3{ rotatedMoveVec.x, 0.0f, rotatedMoveVec.y } *deltaTime * 5.0f;
        cpntTrans.addPos(deltaPos);
        if ( rotatedMoveVec.x != 0.0f || rotatedMoveVec.y != 0.0f ) {  // If moved position
            cpntTrans.setQuat(dal::rotateQuat(glm::quat{}, atan2(rotatedMoveVec.x, rotatedMoveVec.y), glm::vec3{ 0.0f, 1.0f, 0.0f }));

            animModel.m_animState.setSelectedAnimeIndex(1);

            const auto moveSpeed = glm::length(rotatedMoveVec);
            const auto animeSpeed = 1.0f / moveSpeed;
            constexpr float epsilon = 0.0001f;
            if ( epsilon > animeSpeed && animeSpeed > -epsilon ) {  // animeSpeed is near zero than epsion.
                animModel.m_animState.setTimeScale(0.0f);
            }
            else {
                animModel.m_animState.setTimeScale(2.0f / animeSpeed);
            }
        }
    }

    void processCharaHeight(dal::cpnt::Transform& transform, dal::SceneGraph& scene) {
        constexpr float RAY_Y_OFFSET = 2;
        const auto ray = dal::Segment{ transform.getPos() + glm::vec3{ 0, RAY_Y_OFFSET, 0 }, glm::vec3{ 0, -10, 0 } };

        const auto result = scene.doRayCasting(ray);
        if ( !result ) {
            return;
        }

        const auto floorDist = result->m_distance - RAY_Y_OFFSET;
        transform.addPos(0.f, -floorDist, 0.f);
    }

    dal::cpnt::Transform& getPlayerTransform(dal::SceneGraph& scene) {
        return scene.m_entities.get<dal::cpnt::Transform>(scene.m_player);
    }

    dal::cpnt::AnimatedModel& getPlayerModel(dal::SceneGraph& scene) {
        return scene.m_entities.get<dal::cpnt::AnimatedModel>(scene.m_player);
    }

}


// ICharaState derived
namespace {

    class CharaIdleState : public dal::ICharaState {

    public:
        CharaIdleState(dal::FPSEulerCamera& camera, dal::SceneGraph& scene)
            : ICharaState(camera, scene)
        {

        }

        virtual void enter(void) override {
            auto& model = this->m_scene.m_entities.get<dal::cpnt::AnimatedModel>(this->m_scene.m_player);
            model.m_animState.setSelectedAnimeIndex(2);
        }

        virtual void exit(void) override {

        }

        virtual void process(const float deltaTime, const dal::MoveInputInfo& info) override {
            auto& transform = getPlayerTransform(this->m_scene);

            processCharaHeight(transform, this->m_scene);
            applybindingCameraToModel(this->m_camera, deltaTime, info, transform.getPos(), transform.getPos());
        }

        virtual dal::ICharaState* exec(const float deltaTime, const dal::MoveInputInfo& info) override;

    };


    class CharaWalkState : public dal::ICharaState {

    private:
        glm::vec3 m_lastPos;

    public:
        CharaWalkState(dal::FPSEulerCamera& camera, dal::SceneGraph& scene)
            : ICharaState(camera, scene)
        {

        }

        virtual void enter(void) override {
            auto& model = getPlayerModel(this->m_scene);
            auto& transform = getPlayerTransform(this->m_scene);

            model.m_animState.setSelectedAnimeIndex(1);
            this->m_lastPos = transform.getPos();
        }

        virtual void exit(void) override {

        }

        virtual void process(const float deltaTime, const dal::MoveInputInfo& info) override {
            auto& model = getPlayerModel(this->m_scene);
            auto& transform = getPlayerTransform(this->m_scene);

            applyMove(transform, model, this->m_camera, deltaTime, info);
            processCharaHeight(transform, this->m_scene);
            applybindingCameraToModel(this->m_camera, deltaTime, info, transform.getPos(), this->m_lastPos);
            this->m_lastPos = transform.getPos();
        }

        virtual dal::ICharaState* exec(const float deltaTime, const dal::MoveInputInfo& info) override;

    };

}


// Chara states exec functions
namespace {

    dal::ICharaState* CharaIdleState::exec(const float deltaTime, const dal::MoveInputInfo& info) {
        if ( info.hasMovement() ) {
            std::unique_ptr<CharaIdleState> byebye{ this };
            auto newState = new CharaWalkState(this->m_camera, this->m_scene);

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
            auto newState = new CharaIdleState(this->m_camera, this->m_scene);

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

    CharacterState::CharacterState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::FPSEulerCamera& camera, SceneGraph& scene)
        : m_currentState(new CharaIdleState{ camera, scene })
    {

    }

    void CharacterState::update(const float deltaTime, const MoveInputInfo& info) {
        this->m_currentState = this->m_currentState->exec(deltaTime, info);
    }

}
