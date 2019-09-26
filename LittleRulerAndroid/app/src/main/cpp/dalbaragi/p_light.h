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

        void sendUniform(const UniInterfLightedMesh& uniloc, int index) const;

        void clearBuffer(void);
        void startRender(void);
        void finishRender(void);

		const Texture& getTexture(void) {
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

        void sendUniform(const UniInterfLightedMesh& uniloc, int index) const;

        void clearDepthBuffer(void);
        void startRenderShadowmap(const UniInterfGeometry& uniloc);
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
        void sendUniform(const UniInterfLightedMesh& uniloc, int index) const;

    };


    class SpotLight {

    private:
        glm::vec3 m_pos;
        glm::vec3 m_direc;
        glm::vec3 m_color;
        float m_startFade, m_endFade;

    public:
        SpotLight(void);

        void setPos(const float x, const float y, const float z) {
            this->m_pos = glm::vec3{ x, y, z };
        }
        void setPos(const glm::vec3& v) {
            this->m_pos = v;
        }

        void setStartFadeDegree(const float degree) {
            this->setStartFadeRadian(glm::radians(degree));
        }
        void setStartFadeRadian(const float radian) {
            this->m_startFade = cos(radian);
        }
        void setEndFadeDegree(const float degree) {
            this->setEndFadeDegree(glm::radians(degree));
        }
        void setEndFadeRadian(const float radian) {
            this->m_endFade = cos(radian);
        }

        void sendUniform(const UniInterfLightedMesh::SpotLight& uniloc) const;

    };

}
