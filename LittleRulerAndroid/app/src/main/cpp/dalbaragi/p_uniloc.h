#pragma once

#include <glm/glm.hpp>

#include "p_dalopengl.h"


namespace dal {

    class SamplerInterf {

    private:
        GLint m_samplerLoc = -1;
        GLint m_flagHas = -1;
        int m_unitIndex = -1;

    public:
        SamplerInterf(void) = default;
        SamplerInterf(const GLint samplerLoc, const GLint flagHasLoc, const unsigned int unitIndex, const bool flagAssert = false);
        void init(const GLint samplerLoc, const GLint flagHasLoc, const unsigned int unitIndex, const bool flagAssert = false);

        GLint getSamplerLoc(void) const;
        void setFlagHas(const bool x) const;
        int getUnitIndex(void) const;

    };


    class UniInterfGeometry {

    private:
        GLint u_projMat = -1;
        GLint u_viewMat = -1;
        GLint u_modelMat = -1;

    public:
        UniInterfGeometry(const GLuint shader);

        void projectMat(const glm::mat4& mat) const;
        void viewMat(const glm::mat4& mat) const;
        void modelMat(const glm::mat4& mat) const;

     };

    class UniInterfMesh : public UniInterfGeometry {

    private:
        GLint u_texScale = -1;

    public:
        UniInterfMesh(const GLuint shader);

        void texScale(const float x, const float y) const;
        void texScale(const glm::vec2& v) const;

    };

    class UniInterfLightedMesh : public UniInterfMesh {

    public:
        class SpotLight {

        private:
            GLint m_pos = -1;
            GLint m_direc = -1;
            GLint m_color = -1;
            GLint m_startFade = -1;
            GLint m_endFade = -1;

            GLint u_projViewMat = -1;
            SamplerInterf m_depthMap;

        public:
            void init(const GLuint shader, const unsigned int index);

            void pos(const float x, const float y, const float z) const;
            void pos(const glm::vec3& v) const;

            void direc(const float x, const float y, const float z) const;
            void direc(const glm::vec3& v) const;

            void color(const float x, const float y, const float z) const;
            void color(const glm::vec3& v) const;

            void startFade(const float v) const;
            void endFade(const float v) const;

            void projViewMat(const glm::mat4& mat) const;
            const SamplerInterf& getDepthMap(void) const {
                return this->m_depthMap;
            }

        };

        class DirecLight {

        private:
            GLint m_direc = -1;
            GLint m_color = -1;

            GLint u_projViewMat = -1;
            SamplerInterf m_depthMap;

        public:
            void init(const GLuint shader, const unsigned int index);

            void direc(const float x, const float y, const float z) const;
            void direc(const glm::vec3& v) const;

            void color(const float x, const float y, const float z) const;
            void color(const glm::vec3& v) const;

            void projViewMat(const glm::mat4& mat) const;
            const SamplerInterf& getDepthMap(void) const {
                return this->m_depthMap;
            }

        };

        class PointLight {

        private:
            GLint m_pos = -1;
            GLint m_color = -1;

        public:
            void init(const GLuint shader, const unsigned int index);

            void pos(const float x, const float y, const float z) const;
            void pos(const glm::vec3& v) const;

            void color(const float x, const float y, const float z) const;
            void color(const glm::vec3& v) const;

        };

    private:
        static constexpr auto k_maxDlight = 3;
        static constexpr auto k_maxPlight = 3;
        static constexpr auto k_maxSlight = 3;

        GLint uViewPos;
        GLint uBaseAmbient;
        GLint uDlightCount;
        GLint uPlightCount;
        GLint u_slightCount;

        GLint u_roughness;
        GLint u_metallic;

        GLint u_fogMaxPointInvSqr;
        GLint u_fogColor;

        SamplerInterf u_environmentMap;

    public:
        SpotLight u_slights[k_maxSlight];
        DirecLight u_dlights[k_maxDlight];
        PointLight u_plights[k_maxPlight];

    public:
        UniInterfLightedMesh(const GLuint shader);

        void viewPos(const float x, const float y, const float z) const;
        void viewPos(const glm::vec3& v) const;

        void baseAmbient(const float x, const float y, const float z) const;
        void baseAmbient(const glm::vec3& v) const;

        void dlightCount(const unsigned int x) const;
        void plightCount(const unsigned int x) const;
        void slightCount(const unsigned int x) const;

        void roughness(const float v) const;
        void metallic(const float v) const;

        void fogMaxPoint(const float x) const;
        void fogMaxPointAsInfinity(void) const;

        void fogColor(const float x, const float y, const float z) const;
        void fogColor(const glm::vec3& v) const;

        const SamplerInterf& getEnvironmentMap(void) const;

    };


    class UniInterfAnime {

    private:
        static constexpr unsigned int k_maxNumJoints = 30;
        GLint u_jointTransforms[k_maxNumJoints] = { -1 };

    public:
        UniInterfAnime(const GLuint shader);

        void jointTransforms(const unsigned int index, const glm::mat4& mat) const;

    };

    class UniInterfPlaneClip {

    private:
        GLint u_doClip = -1, u_clipPlane = -1;

    public:
        UniInterfPlaneClip(const GLuint shader);

        void flagDoClip(const bool x) const;
        void clipPlane(const glm::vec4& plane) const;
        void clipPlane(const float x, const float y, const float z, const float w) const;

    };


    class UnilocGeneral {

        //////// Vars ////////

    public:
        UniInterfLightedMesh m_lightedMesh;
        UniInterfPlaneClip m_planeClip;

    private:
        SamplerInterf u_diffuseMap;

        //////// Funcs ////////

    public:
        UnilocGeneral(const GLuint shader);

        const SamplerInterf& getDiffuseMapLoc(void) const;

    };

    class UnilocOverlay {

        //////// Vars ////////

    private:
        GLint uPoint1;
        GLint uPoint2;

        GLint mUpsideDown_maskMap;
        GLint m_upsideDown_diffuseMap;

        GLint u_texOffset;
        GLint u_texScale;

        GLint uColor;

        SamplerInterf m_diffuseMap, m_maskMap;

    public:
        UnilocOverlay(const GLuint shader);

        const SamplerInterf& getDiffuseMap(void) const;
        const SamplerInterf& getMaskMap(void) const;

        void texOffset(const float x, const float y) const;
        void texOffset(const glm::vec2& v) const;
        void texScale(const float x, const float y) const;
        void texScale(const glm::vec2& v) const;

        void point1(const glm::vec2& v) const;
        void point2(const glm::vec2& v) const;
        void upsideDownDiffuseMap(const bool x) const;
        void upsideDownMaskMap(const bool x) const;
        void color(const glm::vec4& v) const;

    };

    class UnilocFScreen {

        //////// Vars ////////

    private:
        SamplerInterf m_texture;

        //////// Funcs ////////

    public:
        UnilocFScreen(const GLuint shader);

        const SamplerInterf& getTexture(void) const;

    };

    class UnilocDepthmp {

    public:
        UniInterfGeometry m_geometry;

    public:
        UnilocDepthmp(const GLuint shader);

    };

    class UnilocWaterry {

        //////// Vars ////////

    public:
        UniInterfLightedMesh m_lightedMesh;

    private:
        SamplerInterf m_bansaTex;
        SamplerInterf m_gooljulTex;
        SamplerInterf m_dudvMap;
        SamplerInterf m_normalMap;
        SamplerInterf m_depthMap;

        GLint u_dudvMoveFactor;
        GLint u_waveStrength;
        GLint u_darkestDepthPoint;
        GLint u_reflectivity;
        GLint u_deepColor;

        //////// Funcs ////////

    public:
        UnilocWaterry(const GLuint shader);

        void dudvFactor(const float x) const;
        void waveStrength(const float x) const;
        void darkestDepthPoint(const float x) const;
        void reflectivity(const float x) const;
        void deepColor(const float x, const float y, const float z) const;
        void deepColor(const glm::vec3& v) const;

        const SamplerInterf& getReflectionTex(void) const;
        const SamplerInterf& getRefractionTex(void) const;
        const SamplerInterf& getDUDVMap(void) const;
        const SamplerInterf& getNormalMap(void) const;
        const SamplerInterf& getDepthMap(void) const;

    };

    class UnilocAnimate : public UnilocGeneral {

        //////// Funcs ////////

    public:
        UniInterfAnime m_anime;

    public:
        UnilocAnimate(const GLuint shader);

    };

    class UnilocDepthAnime {

    public:
        UniInterfGeometry m_geometry;
        UniInterfAnime m_anime;

    public:
        UnilocDepthAnime(const GLuint shader);

    };

    class UnilocSkybox {

    public:
        UniInterfGeometry m_geometry;

    private:
        GLint u_fogColor;
        SamplerInterf u_skyboxTex;

    public:
        UnilocSkybox(const GLuint shader);

        void fogColor(const float x, const float y, const float z) const;
        void fogColor(const glm::vec3& v) const;

        const SamplerInterf& getSkyboxTexLoc(void) const;

    };

}