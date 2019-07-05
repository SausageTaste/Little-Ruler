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
        void init(const GLuint shader, const GLint sampler, const GLint flagHas);
        void init(const GLuint shader, const GLint sampler);

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

    private:
        GLint uDlightProjViewMat[3];

        GLint uViewPos;
        GLint uBaseAmbient;
        GLint uDlightCount;
        GLint uPlightCount;

        GLint uShininess;
        GLint uSpecularStrength;

        GLint uDlightDirecs[3];
        GLint uDlightColors[3];
        GLint uDlightDepthMap[3];

        GLint uPlightPoses[3];
        GLint uPlightColors[3];
        GLint uPlightMaxDists[3];

    public:
        UniInterfLightedMesh(const GLuint shader);

        void viewPos(const float x, const float y, const float z) const;
        void viewPos(const glm::vec3& v) const;

        void baseAmbient(const float x, const float y, const float z) const;
        void baseAmbient(const glm::vec3& v) const;

        void dlightCount(const unsigned int x) const;
        void plightCount(const unsigned int x) const;
        void shininess(const float x) const;
        void specularStrength(const float x) const;

        void dlightDirec(const unsigned int index, const float x, const float y, const float z) const;
        void dlightDirec(const unsigned int index, const glm::vec3& v) const;

        void dlightColor(const unsigned int index, const float x, const float y, const float z) const;
        void dlightColor(const unsigned int index, const glm::vec3& v) const;

        GLint getDlightDepthMap(const unsigned int index) const;
        void dlightProjViewMat(const unsigned int index, glm::mat4& mat) const;

        void plightPos(const unsigned int index, const float x, const float y, const float z) const;
        void plightPos(const unsigned int index, const glm::vec3& v) const;

        void plightColor(const unsigned int index, const float x, const float y, const float z) const;
        void plightColor(const unsigned int index, const glm::vec3& v) const;

        void plightMaxDist(const unsigned int index, const float x) const;

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
        GLint u_diffuseMap;

        //////// Funcs ////////

    public:
        UnilocGeneral(const GLuint shader);

        GLint getDiffuseMapLoc(void) const;

    };

    class UnilocOverlay {

        //////// Vars ////////

    public:
        GLint uPoint1;
        GLint uPoint2;

        GLint mUpsideDown_maskMap;
        GLint m_upsideDown_diffuseMap;

        // Fragment shader

        GLint uColor;

        GLint mDiffuseMap;
        GLint mHasDiffuseMap;

        GLint mMaskMap;
        GLint mHasMaskMap;

        //////// Funcs ////////

    public:
        UnilocOverlay(const GLuint shader);

    };

    class UnilocFScreen {

        //////// Vars ////////

    public:
        GLint iPosition;
        GLint iTexCoord;

        // Fragment shader

        GLint uTexture;

        //////// Funcs ////////

    public:
        UnilocFScreen(const GLuint shader);

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

    public:
        GLint u_bansaTex;
        GLint u_gooljulTex;
        GLint u_dudvMap;
        GLint u_normalMap;
        GLint u_dudvMoveFactor;

        //////// Funcs ////////

    public:
        UnilocWaterry(const GLuint shader);

    };

    class UnilocAnimate : public UnilocGeneral {

        //////// Funcs ////////

    public:
        UniInterfAnime m_anime;

    public:
        UnilocAnimate(const GLuint shader);

    };

}