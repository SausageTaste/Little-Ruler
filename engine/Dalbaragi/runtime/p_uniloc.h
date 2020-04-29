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
        GLint u_envmapPos = -1;
        SamplerInterf u_envmap;

        GLint u_numPlanes = -1;
        GLint u_planes = -1;

    public:
        void set(const GLuint shader);

        void envmapPos(const float x, const float y, const float z) const {
            glUniform3f(this->u_envmapPos, x, y, z);
        }
        void envmapPos(const glm::vec3& v) const {
            this->envmapPos(v.x, v.y, v.z);
        }

        auto& envmap(void) const {
            return this->u_envmap;
        }

        void numPlanes(const int x) const {
            glUniform1i(this->u_numPlanes, x);
        }
        void plane(const int index, const float x, const float y, const float z, const float w) const {
            glUniform4f(this->u_planes, x, y, z, w);
        }
        void plane(const int index, const glm::vec4& v) const {
            this->plane(this->u_planes, v.x, v.y, v.z, v.w);
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
        GLint u_plight_maxDists = -1;

        GLint u_dlight_direcs = -1;
        GLint u_dlight_colors = -1;
        GLint u_dlight_projViewMat = -1;
        SamplerInterf u_dlight_shadowmap[3];

        GLint u_slight_poses = -1;
        GLint u_slight_direcs = -1;
        GLint u_slight_colors = -1;
        GLint u_slight_maxDists = -1;
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
        void plight_maxDist(const unsigned i, const float x) const {
            glUniform1f(this->u_plight_maxDists + i, x);
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
        void slight_maxDist(const unsigned i, const float x) const {
            glUniform1f(this->u_slight_maxDists + i, x);
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
        SamplerInterf u_diffuseMap, u_roughnessMap, u_metallicMap, u_normalMap;

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
        auto& normalMap(void) const {
            return this->u_normalMap;
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

    class UniRender_Water {

    public:
        UniInterf_Lighting i_lighting;

    private:
        GLint u_viewPos = -1;

        GLint u_projMat = -1;
        GLint u_viewMat = -1;
        GLint u_modelMat = -1;

        GLint u_texScale = -1;

        SamplerInterf u_bansaImg;
        SamplerInterf u_gooljulImg;
        SamplerInterf u_dudvMap;
        SamplerInterf u_normalMap;
        SamplerInterf u_depthMap;

        GLint u_dudvMoveFactor = -1;
        GLint u_waveStrength = -1;
        GLint u_darkestDepthPoint = -1;
        GLint u_reflectance = -1;
        GLint u_deepColor = -1;

    public:
        void set(const GLuint shader);

        void viewPos(const float x, const float y, const float z) const {
            glUniform3f(this->u_viewPos, x, y, z);
        }
        void viewPos(const glm::vec3& v) const {
            this->viewPos(v.x, v.y, v.z);
        }

        void projMat(const glm::mat4& mat) const {
            sendMatrix(this->u_projMat, mat);
        }
        void viewMat(const glm::mat4& mat) const {
            sendMatrix(this->u_viewMat, mat);
        }
        void modelMat(const glm::mat4& mat) const {
            sendMatrix(this->u_modelMat, mat);
        }

        void texScale(const float x, const float y) const {
            glUniform2f(this->u_texScale, x, y);
        }
        void texScale(const glm::vec2& v) const {
            this->texScale(v.x, v.y);
        }

        auto& reflectionImage(void) const {
            return this->u_bansaImg;
        }
        auto& refractionImage(void) const {
            return this->u_gooljulImg;
        }
        auto& dudvMap(void) const {
            return this->u_dudvMap;
        }
        auto& normalMap(void) const {
            return this->u_normalMap;
        }
        auto& depthMap(void) const {
            return this->u_depthMap;
        }

        void dudvMoveFactor(const float v) const {
            glUniform1f(this->u_dudvMoveFactor, v);
        }
        void waveStrength(const float v) const {
            glUniform1f(this->u_waveStrength, v);
        }
        void darkestDepthPoint(const float v) const {
            glUniform1f(this->u_darkestDepthPoint, v);
        }
        void reflectance(const float v) const {
            glUniform1f(this->u_reflectance, v);
        }
        void deepColor(const float x, const float y, const float z) const {
            glUniform3f(this->u_deepColor, x, y, z);
        }
        void deepColor(const glm::vec3& v) const {
            this->deepColor(v.x, v.y, v.z);
        }

    };

    class UniRender_Skybox {

    public:
        UniInterf_Lighting i_lighting;

    private:
        GLint u_projViewMat = -1;
        GLint u_modelMat = -1;

        GLint u_viewPos = -1;
        GLint u_viewPosActual = -1;
        SamplerInterf u_skyboxTex;

    public:
        void set(const GLuint shader);

        void projViewMat(const glm::mat4& mat) const {
            sendMatrix(this->u_projViewMat, mat);
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
        void viewPosActual(const float x, const float y, const float z) const {
            glUniform3f(this->u_viewPosActual, x, y, z);
        }
        void viewPosActual(const glm::vec3& v) const {
            this->viewPosActual(v.x, v.y, v.z);
        }
        auto& skyboxTex(void) const {
            return this->u_skyboxTex;
        }

    };

    class UniRender_Overlay {

    private:
        GLint u_bottomLeft;
        GLint u_rectSize;

        GLint u_texOffset;
        GLint u_texScale;

        GLint u_yFlip_colorMap;
        GLint u_yFlip_maskMap;

        GLint u_colorDefault;

        SamplerInterf u_colorMap, u_maskMap;

    public:
        void set(const GLuint shader);

        void bottomLeft(const float x, const float y) const {
            glUniform2f(this->u_bottomLeft, x, y);
        }
        void bottomLeft(const glm::vec2& v) const {
            this->bottomLeft(v.x, v.y);
        }
        void rectSize(const float x, const float y) const {
            glUniform2f(this->u_rectSize, x, y);
        }
        void rectSize(const glm::vec2& v) const {
            this->rectSize(v.x, v.y);
        }

        void texOffset(const float x, const float y) const {
            glUniform2f(this->u_texOffset, x, y);
        }
        void texOffset(const glm::vec2& v) const {
            this->texOffset(v.x, v.y);
        }
        void texScale(const float x, const float y) const {
            glUniform2f(this->u_texScale, x, y);
        }
        void texScale(const glm::vec2& v) const {
            this->texScale(v.x, v.y);
        }

        void yFlip_colorMap(const bool v) const {
            sendBool(this->u_yFlip_colorMap, v);
        }
        void yFlip_maskMap(const bool v) const {
            sendBool(this->u_yFlip_maskMap, v);
        }

        void colorDefault(const float r, const float g, const float b, const float a) const {
            glUniform4f(this->u_colorDefault, r, g, b, a);
        }
        void colorDefault(const glm::vec4& v) const {
            this->colorDefault(v.x, v.y, v.z, v.w);
        }

        auto& colorMap(void) const {
            return this->u_colorMap;
        }
        auto& maskMap(void) const {
            return this->u_maskMap;
        }

    };

}
