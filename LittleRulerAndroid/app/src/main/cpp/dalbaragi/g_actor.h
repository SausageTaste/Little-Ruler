#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    class Model;


    class Camera {

    private:
        glm::vec2 m_viewDirec;
        glm::vec3 m_pos;

    public:
        glm::mat4 makeViewMat(void) const;

        glm::vec3 getPos(void) const;
        void setPos(const float x, const float y, const float z);
        void setPos(const glm::vec3& pos);
        void addPos(const float x, const float y, const float z);
        void addPos(const glm::vec3& pos);

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
        Camera* m_camera = nullptr;
        ActorInfo* m_actor = nullptr;
        Model* m_model = nullptr;

    public:
        Player(void) = default;
        Player(Camera* camera, ActorInfo* actor, Model* model);

        Camera* replaceCamera(Camera* const camera);
        ActorInfo* replaceActor(ActorInfo* const actor);
        Model* replaceModel(Model* const model);

    };

}