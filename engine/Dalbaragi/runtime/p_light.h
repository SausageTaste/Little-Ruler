#pragma once

#include <string>

#include <glm/glm.hpp>

#include "p_uniloc.h"
#include "p_meshStatic.h"


namespace dal {

    class DepthmapForLights {

    private:
        GLuint m_fbo = 0;
        unsigned int m_width = 0, m_height = 0;
        Texture m_depthTex;

    public:
        DepthmapForLights(const DepthmapForLights&) = delete;
        DepthmapForLights& operator=(const DepthmapForLights&) = delete;

    public:
        DepthmapForLights(void);
        ~DepthmapForLights(void);
        DepthmapForLights(DepthmapForLights&& other) noexcept;
        DepthmapForLights& operator=(DepthmapForLights&&) noexcept;

        void sendUniform(const SamplerInterf& uniloc) const;

        void clearBuffer(void);
        void startRender(void);
        void finishRender(void);

		const Texture& getTexture(void) const {
			return this->m_depthTex;
		}

    };


    class ILight {

    public:
        std::string m_name;
        glm::vec3 m_color{ 1.0f, 1.0f, 1.0f };

    };


    class DirectionalLight : public ILight {

    private:
        glm::vec3 m_pos;
        glm::vec3 m_direction{ -0.3f, -1.0f, -1.0f };  // This must be always normalized.
        float m_halfProjBoxEdgeLen;
        DepthmapForLights m_shadowMap;

    public:
        DirectionalLight(void);

        void setDirectin(const glm::vec3& v);
        void setDirectin(const float x, const float y, const float z) {
            this->setDirectin(glm::vec3{ x, y, z });
        }
        void setPos(const glm::vec3& v) {
            this->m_pos = v;
        }
        void setPos(const float x, const float y, const float z) {
            this->m_pos.x = x;
            this->m_pos.y = y;
            this->m_pos.z = z;
        }
        const glm::vec3& getDirection(void) const {
            return this->m_direction;
        }

        void sendUniform(const UniInterfLightedMesh::DirecLight& uniloc) const;
        void sendUniform(const unsigned index, const UniInterf_Lighting& uniloc) const;

        void clearDepthBuffer(void);
        void startRenderShadowmap(const UniInterfGeometry& uniloc);
        void startRenderShadowmap(const UniRender_StaticDepth& uniloc);
        void startRenderShadowmap(const UniRender_AnimatedDepth& uniloc);
        void finishRenderShadowmap(void);

        glm::mat4 makeProjMat(void) const;
        glm::mat4 makeViewMat(void) const;

		const Texture& getDepthTex(void) {
			return this->m_shadowMap.getTexture();
		}

    };


    class PointLight : public ILight {

    public:
        glm::vec3 mPos;
        float mMaxDistance = 5.0f;

    public:
        void sendUniform(const UniInterfLightedMesh::PointLight& uniloc) const;
        void sendUniform(unsigned index, const UniInterf_Lighting& uniloc) const;

    };


    class SpotLight {

    private:
        glm::vec3 m_pos;
        glm::vec3 m_direc;
        glm::vec3 m_color;
        float m_endFadeRadians;
        float m_startFade, m_endFade;

        DepthmapForLights m_shadowMap;

    public:
        SpotLight(void);

        void setPos(const float x, const float y, const float z) {
            this->m_pos = glm::vec3{ x, y, z };
        }
        void setPos(const glm::vec3& v) {
            this->m_pos = v;
        }
        void setDirec(const float x, const float y, const float z) {
            this->setDirec(glm::vec3{ x, y, z });
        }
        void setDirec(const glm::vec3& v) {
            this->m_direc = glm::normalize(v);
        }
        void setColor(const float x, const float y, const float z) {
            this->m_color = glm::vec3{ x, y, z };
        }
        void setColor(const glm::vec3& v) {
            this->m_color = v;
        }

        void setStartFadeDegree(const float degree) {
            this->m_startFade = cos(glm::radians(degree));
        }
        void setEndFadeDegree(const float degree) {
            this->m_endFadeRadians = glm::radians(degree);
            this->m_endFade = cos(this->m_endFadeRadians);
        }

        void sendUniform(const UniInterfLightedMesh::SpotLight& uniloc) const;
        void sendUniform(const UniInterf_Lighting& uniloc, const unsigned index) const;

        // Shadow mapping

        glm::mat4 makeProjMat(void) const;
        glm::mat4 makeViewMat(void) const;

        void clearDepthBuffer(void) {
            this->m_shadowMap.clearBuffer();
        }
        void startRenderShadowmap(const UniRender_StaticDepth& uniloc) {
            uniloc.projMat(this->makeProjMat());
            uniloc.viewMat(this->makeViewMat());
            this->m_shadowMap.startRender();
        }
        void startRenderShadowmap(const UniRender_AnimatedDepth& uniloc) {
            uniloc.projMat(this->makeProjMat());
            uniloc.viewMat(this->makeViewMat());
            this->m_shadowMap.startRender();
        }
        void finishRenderShadowmap(void) {
            this->m_shadowMap.finishRender();
        }

        const dal::Texture* getDepthMap(void) const {
            return &this->m_shadowMap.getTexture();
        }

    };

}
