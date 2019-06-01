#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    class Model;


    class ICamera {

    public:
        glm::vec3 m_pos;

    protected:
        glm::mat4 m_viewMat;

    public:
        virtual ~ICamera(void) = default;
        virtual void updateViewMat(void) = 0;
        const glm::mat4& getViewMat(void) const {
            return this->m_viewMat;
        }

    };


    class EulerCamera : ICamera {

    private:
        glm::vec2 m_viewDirec;
        glm::vec3 m_pos;

    public:
        virtual void updateViewMat(void) override;

        glm::vec2 getViewPlane(void) const;
        void setViewPlane(const float x, const float y);
        void addViewPlane(const float x, const float y);

    private:
        void clampViewDir(void);

    };


    class ActorInfo {

    private:
        std::string m_name;
        bool m_static = true;

    public:
        glm::vec3 m_pos;
        glm::quat m_quat;

    public:
        ActorInfo(void) = default;
        ActorInfo(const std::string& actorName, const bool flagStatic);

        ActorInfo(const ActorInfo&) = default;
        ActorInfo& operator=(const ActorInfo&) = default;

        glm::mat4 getViewMat(void) const;
        void rotate(const float v, const glm::vec3& selector);

    };


    class Player {

    private:
        EulerCamera* m_camera = nullptr;
        ActorInfo* m_actor = nullptr;
        Model* m_model = nullptr;

    public:
        Player(void) = default;
        Player(EulerCamera* camera, ActorInfo* actor, Model* model);

        EulerCamera* replaceCamera(EulerCamera* const camera);
        ActorInfo* replaceActor(ActorInfo* const actor);
        Model* replaceModel(Model* const model);

        EulerCamera* getCamera(void) { assert(nullptr != this->m_camera); return this->m_camera; }
        ActorInfo* getActor(void) { assert(nullptr != this->m_actor); return this->m_actor; }
        Model* getModel(void) { assert(nullptr != this->m_model); return this->m_model; }

    };

}