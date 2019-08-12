#pragma once

#include <string>

#include <glm/glm.hpp>

#include "p_uniloc.h"
#include "p_resource.h"


namespace dal {

    class DepthmapForLights {

    private:
        GLuint mFBO = 0;
        unsigned int width = 0, height = 0;
        Texture m_depthTex;

    private:
        DepthmapForLights(const DepthmapForLights&) = delete;
        DepthmapForLights& operator=(const DepthmapForLights&) = delete;

    public:
        DepthmapForLights(void);
        ~DepthmapForLights(void);
        DepthmapForLights(DepthmapForLights&& other) noexcept;
        DepthmapForLights& operator=(DepthmapForLights&&) noexcept;

        GLuint getTextureID(void);
        const Texture* getDepthMap(void) const;

        void clearBuffer(void);
        void startRender(void);
        void finishRender(void);

    };


    class ILight {

    public:
        std::string m_name;
        glm::vec3 m_color{ 1.0f, 1.0f, 1.0f };

    };


    class DirectionalLight : public ILight {

    private:
        glm::vec3 m_direction{ -0.3f, -1.0f, -1.0f };  // This must be always normalized.

    public:
        float mHalfShadowEdgeSize;
        DepthmapForLights mShadowMap;

    public:
        DirectionalLight(void);

        void setDirectin(const glm::vec3& direction);
        void setDirectin(const float x, const float y, const float z);
        const glm::vec3& getDirection(void) const;

        void sendUniform(const UniInterfLightedMesh& uniloc, int index) const;

        void clearDepthBuffer(void);
        void startRenderShadowmap(const UniInterfGeometry& uniloc);
        void finishRenderShadowmap(void);

        glm::mat4 makeProjMat(void) const;
        glm::mat4 makeViewMat(void) const;
        GLuint getShadowMapTexture(void);
        const Texture* getShadowMap(void) const;

    };


    class PointLight : public ILight {

    public:
        glm::vec3 mPos;
        float mMaxDistance = 5.0f;

    public:
        void sendUniform(const UniInterfLightedMesh& uniloc, int index) const;

    };

}