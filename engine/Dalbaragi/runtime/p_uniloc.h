#pragma once

#include <glm/glm.hpp>

#include "p_dalopengl.h"


namespace dal {

    inline void sendMatrix(const GLint loc, const glm::mat4& mat) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
    }

    inline void sendBool(const GLint loc, const bool x) {
        glUniform1i(loc, x ? 1 : 0);
    }


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

}


// Uniloc Interface
namespace dal {

    class UniInterf_Envmap {

    private:
        SamplerInterf u_envmap;

    public:
        void set(const GLuint shader);

        auto& envmap(void) const {
            return this->u_envmap;
        }

    };

    class UniInterf_Lighting {

    private:
        GLint u_baseAmbient = -1;

        GLint u_roughness = -1;
        GLint u_metallic = -1;

        GLint u_dlightCount = -1;
        GLint u_plightCount = -1;
        GLint u_slightCount = -1;

        GLint u_plight_poses = -1;
        GLint u_plight_colors = -1;

        GLint u_dlight_direcs = -1;
        GLint u_dlight_colors = -1;
        GLint u_dlight_projViewMat = -1;
        SamplerInterf u_dlight_shadowmap[3];

        GLint u_slight_poses = -1;
        GLint u_slight_direcs = -1;
        GLint u_slight_colors = -1;
        GLint u_slight_fadeStart = -1;
        GLint u_slight_fadeEnd = -1;
        GLint u_slight_projViewMat = -1;
        SamplerInterf u_slight_shadowmap[3];

    public:
        void set(const GLuint shader);

        void baseAmbient(const float x, const float y, const float z) const {
            glUniform3f(this->u_baseAmbient, x, y, z);
        }
        void baseAmbient(const glm::vec3& v) const {
            this->baseAmbient(v.x, v.y, v.z);
        }

        void roughness(const float x) const {
            glUniform1f(this->u_roughness, x);
        }
        void metallic(const float x) const {
            glUniform1f(this->u_metallic, x);
        }

        void dlightCount(const int x) const {
            glUniform1i(this->u_dlightCount, x);
        }
        void plightCount(const int x) const {
            glUniform1i(this->u_plightCount, x);
        }
        void slightCount(const int x) const {
            glUniform1i(this->u_slightCount, x);
        }

        void plight_pos(const unsigned i, const float x, const float y, const float z) const {
            glUniform3f(this->u_plight_poses + i, x, y, z);
        }
        void plight_pos(const unsigned i, const glm::vec3& v) const {
            this->plight_pos(i, v.x, v.y, v.z);
        }
        void plight_color(const unsigned i, const float x, const float y, const float z) const {
            glUniform3f(this->u_plight_colors + i, x, y, z);
        }
        void plight_color(const unsigned i, const glm::vec3& v) const {
            this->plight_color(i, v.x, v.y, v.z);
        }

        void dlight_direc(const unsigned i, const float x, const float y, const float z) const {
            glUniform3f(this->u_dlight_direcs + i, x, y, z);
        }
        void dlight_direc(const unsigned i, const glm::vec3& v) const {
            this->dlight_direc(i, v.x, v.y, v.z);
        }
        void dlight_color(const unsigned i, const float x, const float y, const float z) const {
            glUniform3f(this->u_dlight_colors + i, x, y, z);
        }
        void dlight_color(const unsigned i, const glm::vec3& v) const {
            this->dlight_color(i, v.x, v.y, v.z);
        }
        void dlight_projViewMat(const unsigned i, const glm::mat4& mat) const {
            sendMatrix(this->u_dlight_projViewMat + i, mat);
        }
        auto& dlight_shadowmap(const unsigned i) const {
            return this->u_dlight_shadowmap[i];
        }

        void slight_poses(const unsigned i, const float x, const float y, const float z) const {
            glUniform3f(this->u_slight_poses + i, x, y, z);
        }
        void slight_poses(const unsigned i, const glm::vec3& v) const {
            this->slight_poses(i, v.x, v.y, v.z);
        }
        void slight_direcs(const unsigned i, const float x, const float y, const float z) const {
            glUniform3f(this->u_slight_direcs + i, x, y, z);
        }
        void slight_direcs(const unsigned i, const glm::vec3& v) const {
            this->slight_direcs(i, v.x, v.y, v.z);
        }
        void slight_colors(const unsigned i, const float x, const float y, const float z) const {
            glUniform3f(this->u_slight_colors + i, x, y, z);
        }
        void slight_colors(const unsigned i, const glm::vec3& v) const {
            this->slight_colors(i, v.x, v.y, v.z);
        }
        void slight_fadeStart(const unsigned i, const float x) const {
            glUniform1f(this->u_slight_fadeStart + i, x);
        }
        void slight_fadeEnd(const unsigned i, const float x) const {
            glUniform1f(this->u_slight_fadeEnd + i, x);
        }
        void slight_projViewMat(const unsigned i, const glm::mat4& mat) const {
            sendMatrix(this->u_slight_projViewMat + i, mat);
        }
        auto& slight_shadowmap(const unsigned i) const {
            return this->u_slight_shadowmap[i];
        }
    };

    class UniInterf_Lightmap {

    private:
        SamplerInterf u_diffuseMap, u_roughnessMap, u_metallicMap;

    public:
        void set(const GLuint shader);

        auto& diffuseMap(void) const {
            return u_diffuseMap;
        }
        auto& roughnessMap(void) const {
            return u_roughnessMap;
        }
        auto& metallicMap(void) const {
            return u_metallicMap;
        }

    };

    class UniInterf_Skeleton {

    private:
        GLint u_jointTrans = -1;

    public:
        void set(const GLuint shader);

        void jointTrans(const unsigned i, const glm::mat4& mat) const {
            sendMatrix(this->u_jointTrans + i, mat);
        }

    };

}


// Uniloc Render
namespace dal {

    class UniRender_Static {

    public:
        UniInterf_Lighting i_lighting;
        UniInterf_Lightmap i_lightmap;
        UniInterf_Envmap i_envmap;

    private:
        GLint u_projMat = -1;
        GLint u_viewMat = -1;
        GLint u_modelMat = -1;

        GLint u_viewPos = -1;

    public:
        void set(const GLuint shader);

        void projMat(const glm::mat4& mat) const {
            sendMatrix(this->u_projMat, mat);
        }
        void viewMat(const glm::mat4& mat) const {
            sendMatrix(this->u_viewMat, mat);
        }
        void modelMat(const glm::mat4& mat) const {
            sendMatrix(this->u_modelMat, mat);
        }

        void viewPos(const float x, const float y, const float z) const {
            glUniform3f(this->u_viewPos, x, y, z);
        }
        void viewPos(const glm::vec3& v) const {
            this->viewPos(v.x, v.y, v.z);
        }

    };

    class UniRender_Animated {

    public:
        UniInterf_Lighting i_lighting;
        UniInterf_Lightmap i_lightmap;
        UniInterf_Envmap i_envmap;
        UniInterf_Skeleton i_skeleton;

    private:
        GLint u_projMat = -1;
        GLint u_viewMat = -1;
        GLint u_modelMat = -1;

        GLint u_viewPos = -1;

    public:
        void set(const GLuint shader);

        void projMat(const glm::mat4& mat) const {
            sendMatrix(this->u_projMat, mat);
        }
        void viewMat(const glm::mat4& mat) const {
            sendMatrix(this->u_viewMat, mat);
        }
        void modelMat(const glm::mat4& mat) const {
            sendMatrix(this->u_modelMat, mat);
        }

        void viewPos(const float x, const float y, const float z) const {
            glUniform3f(this->u_viewPos, x, y, z);
        }
        void viewPos(const glm::vec3& v) const {
            this->viewPos(v.x, v.y, v.z);
        }

    };

    class UniRender_StaticDepth {

    private:
        GLint u_projMat = -1;
        GLint u_viewMat = -1;
        GLint u_modelMat = -1;

    public:
        void set(const GLuint shader);

        void projMat(const glm::mat4& mat) const {
            sendMatrix(this->u_projMat, mat);
        }
        void viewMat(const glm::mat4& mat) const {
            sendMatrix(this->u_viewMat, mat);
        }
        void modelMat(const glm::mat4& mat) const {
            sendMatrix(this->u_modelMat, mat);
        }

    };

    class UniRender_AnimatedDepth {

    public:
        UniInterf_Skeleton i_skeleton;

    private:
        GLint u_projMat = -1;
        GLint u_viewMat = -1;
        GLint u_modelMat = -1;

    public:
        void set(const GLuint shader);

        void projMat(const glm::mat4& mat) const {
            sendMatrix(this->u_projMat, mat);
        }
        void viewMat(const glm::mat4& mat) const {
            sendMatrix(this->u_viewMat, mat);
        }
        void modelMat(const glm::mat4& mat) const {
            sendMatrix(this->u_modelMat, mat);
        }

    };

    class UniRender_StaticOnWater : public UniRender_Static {

    private:
        GLint u_clipPlane = -1;

    public:
        void set(const GLuint shader);

        void clipPlane(const float x, const float y, const float z, const float w) const {
            glUniform4f(this->u_clipPlane, x, y, z, w);
        }
        void clipPlane(const glm::vec4 plane) const {
            this->clipPlane(plane.x, plane.y, plane.z, plane.w);
        }

    };

    class UniRender_AnimatedOnWater : public UniRender_Animated {

    private:
        GLint u_clipPlane = -1;

    public:
        void set(const GLuint shader);

        void clipPlane(const float x, const float y, const float z, const float w) const {
            glUniform4f(this->u_clipPlane, x, y, z, w);
        }
        void clipPlane(const glm::vec4 plane) const {
            this->clipPlane(plane.x, plane.y, plane.z, plane.w);
        }

    };

    class UniRender_FillScreen {

    private:
        SamplerInterf u_texture;

    public:
        void set(const GLuint shader);

        auto& texture(void) const {
            return this->u_texture;
        }

    };

}


namespace dal {

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

    };


    class UnilocOverlay {

        //////// Vars ////////

    private:
        GLint u_bottLeft;
        GLint u_rectSize;

        GLint u_upsideDown_maskMap;
        GLint u_upsideDown_diffuseMap;

        GLint u_texOffset;
        GLint u_texScale;

        GLint u_color;

        SamplerInterf m_diffuseMap, m_maskMap;

    public:
        UnilocOverlay(const GLuint shader);

        const SamplerInterf& getDiffuseMap(void) const;
        const SamplerInterf& getMaskMap(void) const;

        void texOffset(const float x, const float y) const;
        void texOffset(const glm::vec2& v) const;
        void texScale(const float x, const float y) const;
        void texScale(const glm::vec2& v) const;

        void bottomLeft(const glm::vec2& v) const;
        void rectSize(const glm::vec2& v) const;
        void upsideDownDiffuseMap(const bool x) const;
        void upsideDownMaskMap(const bool x) const;
        void color(const glm::vec4& v) const;

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
