#pragma once

#include <string>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace dal {

    class Model;

    glm::vec3 strangeEuler2Vec(const float x, const float y);
    glm::vec2 vec2StrangeEuler(glm::vec3 v);


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

        virtual void makeReflected(const float planeHeight, glm::vec3& pos, glm::mat4& mat) const = 0;

    };


    class StrangeEuler {

    private:
        float x, y;

    public:
        StrangeEuler(void);
        StrangeEuler(const float x, const float y);

        float getX(void) const;
        float getY(void) const;
        void setX(const float v);
        void setY(const float v);
        void addX(const float v);
        void addY(const float v);

        glm::mat4 makeRotateMat(void) const;

    };


    class StrangeEulerCamera : public ICamera {

    private:
        StrangeEuler m_stranEuler;

    public:
        virtual void updateViewMat(void) override;
        void makeReflected(const float planeHeight, glm::vec3& pos, glm::mat4& mat) const override;

        glm::vec2 getViewPlane(void) const;
        void setViewPlane(const float x, const float y);
        void addViewPlane(const float x, const float y);

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
        StrangeEulerCamera* m_camera = nullptr;
        ActorInfo* m_actor = nullptr;
        Model* m_model = nullptr;

    public:
        Player(void) = default;
        Player(StrangeEulerCamera* camera, ActorInfo* actor, Model* model);

        StrangeEulerCamera* replaceCamera(StrangeEulerCamera* const camera);
        ActorInfo* replaceActor(ActorInfo* const actor);
        Model* replaceModel(Model* const model);

        StrangeEulerCamera* getCamera(void) { assert(nullptr != this->m_camera); return this->m_camera; }
        ActorInfo* getActor(void) { assert(nullptr != this->m_actor); return this->m_actor; }
        Model* getModel(void) { assert(nullptr != this->m_model); return this->m_model; }

    };

}