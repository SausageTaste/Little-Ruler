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


    ICharaState::ICharaState(dal::ICamera& camera, SceneGraph& scene)
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
                camera.addPos(deltaPos * CAM_ROTATE_SPEED_INV);
            }

            {
                const auto obj2CamVec = camera.pos() - camOrigin;
                const auto len = glm::length(obj2CamVec);
                auto obj2CamSEuler = dal::vec2fpsEuler(obj2CamVec);

                obj2CamSEuler.addX(totalMoveInfo.m_view.x);
                obj2CamSEuler.addY(-totalMoveInfo.m_view.y);
                //obj2CamSEuler.clampY(glm::radians(50.0f), glm::radians(80.0f));

                obj2CamSEuler.clampY(glm::radians(-MAX_Y_DEGREE), glm::radians(MAX_Y_DEGREE));
                const auto rotatedVec = dal::fpsEuler2vec(obj2CamSEuler);
                camera.setPos(camOrigin + rotatedVec * len);
            }

            {
                // It break when OBJ_CAM_DISTANCE's value is lower than 3.

                const auto cam2ObjVec = camOrigin - camera.pos();
                const auto cam2ObjSEuler = dal::vec2fpsEuler(cam2ObjVec);
                camera.setViewPlane(cam2ObjSEuler.x(), cam2ObjSEuler.y());

                camera.setPos(camOrigin - dal::resizeOnlyXZ(cam2ObjVec, OBJ_CAM_DISTANCE));
            }
        }

        camera.updateViewMat();
    }

    void applyMove(dal::cpnt::Transform& cpntTrans, dal::cpnt::AnimatedModel& animModel, const dal::ICamera& camera,
        const float deltaTime, const dal::MoveInputInfo& totalMoveInfo)
    {
        constexpr float CAM_ROTATE_SPEED_INV = 1.0f;
        static_assert(0.0f <= CAM_ROTATE_SPEED_INV && CAM_ROTATE_SPEED_INV <= 1.0f);

        // const auto camViewVec = dal::strangeEuler2Vec(camera.eulerAngles());
        // const auto rotatorAsCamX = glm::rotate(glm::mat4{ 1.0f }, camera.eulerAngles().getX(), glm::vec3{ 0.0f, 1.0f, 0.0f });
        const auto rotatedMoveVec = dal::rotateVec2(glm::vec2{ totalMoveInfo.m_move.x, totalMoveInfo.m_move.y }, camera.calcDirectionXZ());

        const auto deltaPos = glm::vec3{ rotatedMoveVec.x, 0.0f, rotatedMoveVec.y } *deltaTime * 5.0f;
        cpntTrans.addPos(deltaPos);
        if ( rotatedMoveVec.x != 0.0f || rotatedMoveVec.y != 0.0f ) {  // If moved position
            cpntTrans.setQuat(dal::rotateQuat(glm::quat{1, 0, 0, 0}, atan2(rotatedMoveVec.x, rotatedMoveVec.y), glm::vec3{ 0.0f, 1.0f, 0.0f }));

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


    constexpr float HEIGHT_RAY_Y_OFFSET = 1.2;
    constexpr float STOP_FALLING_OFFSET = 0.1;
    constexpr float SNAP_TO_FLOOR_HEIGHT = 0.5;

    auto makeHeightRay(const glm::vec3& pos) {
        return dal::Segment{ pos + glm::vec3{ 0, HEIGHT_RAY_Y_OFFSET, 0 }, glm::vec3{ 0, -10, 0 } };;
    }

    std::optional<float> findDistanceToFloor(dal::cpnt::Transform& transform, dal::SceneGraph& scene) {
        const auto result = scene.doRayCasting(::makeHeightRay(transform.getPos()));
        if ( !result )
            return std::nullopt;
        else
            return result->m_distance - HEIGHT_RAY_Y_OFFSET;
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

    private:
        dal::ICharaState* m_sWalk = nullptr;
        dal::ICharaState* m_sFall = nullptr;

    public:
        CharaIdleState(dal::ICamera& camera, dal::SceneGraph& scene)
            : ICharaState(camera, scene)
        {

        }

        void registerNext(dal::ICharaState& sWalk, dal::ICharaState& fall) {
            this->m_sWalk = &sWalk;
            this->m_sFall = &fall;
        }

        virtual void enter(void) override {
            dalVerbose("Enter idle");

            auto& model = this->m_scene.m_entities.get<dal::cpnt::AnimatedModel>(this->m_scene.m_player);
            model.m_animState.setSelectedAnimeIndex(2);

            auto& transform = getPlayerTransform(this->m_scene);
            const auto height = ::findDistanceToFloor(transform, this->m_scene);
            if ( height.value_or(SNAP_TO_FLOOR_HEIGHT + 1) <= SNAP_TO_FLOOR_HEIGHT ) {
                transform.addPos(0, -height.value(), 0);
            }
        }

        virtual void exit(void) override {

        }

        virtual void process(const float deltaTime, const dal::MoveInputInfo& info) override {
            auto& transform = getPlayerTransform(this->m_scene);
        }

        virtual dal::ICharaState* exec(const float deltaTime, const dal::MoveInputInfo& info) override {
            dal::ICharaState* next = this;

            auto& transform = getPlayerTransform(this->m_scene);
            const auto height = ::findDistanceToFloor(transform, this->m_scene);

            if ( height.value_or(SNAP_TO_FLOOR_HEIGHT + 1) > SNAP_TO_FLOOR_HEIGHT ) {
                next = this->m_sFall;

                this->exit();
                next->enter();
            }
            else if ( info.hasMovement() ) {
                this->exit();
                this->m_sWalk->enter();
                next = this->m_sWalk;
            }

            next->process(deltaTime, info);
            return next;
        }

    };


    class CharaWalkState : public dal::ICharaState {

    private:
        dal::ICharaState* m_sIdle = nullptr;
        dal::ICharaState* m_sFall = nullptr;

        glm::vec3 m_lastPos{};

    public:
        CharaWalkState(dal::ICamera& camera, dal::SceneGraph& scene)
            : ICharaState(camera, scene)
        {

        }

        void registerNext(dal::ICharaState& idle, dal::ICharaState& fall) {
            this->m_sIdle = &idle;
            this->m_sFall = &fall;
        }

        virtual void enter(void) override {
            dalVerbose("Enter walk");

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

            ::applyMove(transform, model, this->m_camera, glm::clamp<float>(deltaTime, 0, 1.0 / 20.0), info);
            const auto height = ::findDistanceToFloor(transform, this->m_scene);
            if ( height.value_or(SNAP_TO_FLOOR_HEIGHT + 1) <= SNAP_TO_FLOOR_HEIGHT ) {
                transform.addPos(0, -height.value(), 0);
            }

            this->m_lastPos = transform.getPos();
        }

        virtual dal::ICharaState* exec(const float deltaTime, const dal::MoveInputInfo& info) override {
            dal::ICharaState* next = this;

            auto& transform = getPlayerTransform(this->m_scene);
            const auto height = ::findDistanceToFloor(transform, this->m_scene);

            if ( height.value_or(SNAP_TO_FLOOR_HEIGHT + 1) > SNAP_TO_FLOOR_HEIGHT ) {
                next = this->m_sFall;

                this->exit();
                next->enter();
            }
            else if ( !info.hasMovement() ) {
                next = this->m_sIdle;

                this->exit();
                next->enter();
            }

            next->process(deltaTime, info);
            return next;
        }

    };


    class CharaFallingState : public dal::ICharaState {

    private:
        dal::ICharaState* m_sIdle = nullptr;
        dal::ICharaState* m_sWalk = nullptr;

        glm::vec3 m_lastPos{};
        double m_fallStartSec = 0;

    public:
        CharaFallingState(dal::ICamera& camera, dal::SceneGraph& scene)
            : ICharaState(camera, scene)
        {

        }

        void registerNext(dal::ICharaState& idle, dal::ICharaState& walk) {
            this->m_sIdle = &idle;
            this->m_sWalk = &walk;
        }

        virtual void enter(void) override {
            dalVerbose("Enter falling");

            auto& model = getPlayerModel(this->m_scene);
            auto& transform = getPlayerTransform(this->m_scene);

            model.m_animState.setSelectedAnimeIndex(1);

            this->m_lastPos = transform.getPos();
            this->m_fallStartSec = dal::getTime_sec();
        }

        virtual void exit(void) override {

        }

        virtual void process(const float deltaTime, const dal::MoveInputInfo& info) override {
            auto& model = getPlayerModel(this->m_scene);
            auto& transform = getPlayerTransform(this->m_scene);

            ::applyMove(transform, model, this->m_camera, deltaTime, info);

            const auto accumTime = dal::getTime_sec() - this->m_fallStartSec;
            const auto fallDist = 2 * accumTime;
            const auto fallDistClamped = glm::clamp<float>(fallDist, 0, HEIGHT_RAY_Y_OFFSET * 0.9);
            transform.addPos(0, -fallDistClamped, 0);

            this->m_lastPos = transform.getPos();
        }

        virtual dal::ICharaState* exec(const float deltaTime, const dal::MoveInputInfo& info) override {
            dal::ICharaState* next = this;

            auto trans = ::getPlayerTransform(this->m_scene);
            const auto height = ::findDistanceToFloor(trans, this->m_scene);
            if ( height.value_or(STOP_FALLING_OFFSET + 1) <= STOP_FALLING_OFFSET ) {
                next = info.hasMovement() ? this->m_sWalk : this->m_sIdle;

                this->exit();
                next->enter();
            }

            next->process(deltaTime, info);
            return next;
        }

    };

}


namespace dal::cpnt {

    CharacterState::CharacterState(cpnt::Transform& transform, cpnt::AnimatedModel& model, dal::ICamera& camera, SceneGraph& scene) {
        const auto idle = new CharaIdleState(camera, scene);
        const auto walk = new CharaWalkState(camera, scene);
        const auto fall = new CharaFallingState(camera, scene);

        idle->registerNext(*walk, *fall);
        walk->registerNext(*idle, *fall);
        fall->registerNext(*idle, *walk);

        this->m_states.push_back(idle);
        this->m_states.push_back(walk);
        this->m_states.push_back(fall);

        this->m_currentState = this->m_states.front();
    }

    CharacterState::~CharacterState(void) {
        for ( auto p : this->m_states ) {
            delete p;
        }
    }

    void CharacterState::update(const float deltaTime, const MoveInputInfo& info) {
        this->m_currentState = this->m_currentState->exec(deltaTime, info);
    }

}
