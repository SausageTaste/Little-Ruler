#pragma once

#include <glm/glm.hpp>

#include "p_dalopengl.h"


namespace dal {

    class UniInterfGeometry {

    private:
        GLint u_projMat = -1;
        GLint u_viewMat = -1;
        GLint u_modelMat = -1;

    protected:
        void init(const GLuint shader);

    public:
        void projectMat(const glm::mat4& mat) const;
        void viewMat(const glm::mat4& mat) const;
        void modelMat(const glm::mat4& mat) const;

     };

    class UniInterfMesh : public UniInterfGeometry {

    private:
        GLint u_texScale = -1;

    public:
        void init(const GLuint shader);

        void texScale(const float x, const float y) const;
        void texScale(const glm::vec2& v) const;

    };

    class UniInterfLight {

    };


    class UnilocGeneral : public UniInterfMesh {

        //////// Vars ////////

    public:
        GLint u_doClip;
        GLint u_clipPlane;

        GLint uDlightProjViewMat[3];

        // Fragment shader

        GLint uViewPos;
        GLint uBaseAmbient;
        GLint uDlightCount;
        GLint uPlightCount;

        GLint uShininess;
        GLint uSpecularStrength;

        GLint uDiffuseColor;
        GLint uHasDiffuseMap;
        GLint uDiffuseMap;

        GLint uDlightDirecs[3];
        GLint uDlightColors[3];
        GLint uDlightDepthMap[3];

        GLint uPlightPoses[3];
        GLint uPlightColors[3];
        GLint uPlightMaxDists[3];

        //////// Funcs ////////

    public:
        void init(const GLuint shader);

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
        void init(const GLuint shader);

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
        void init(const GLuint shader);

    };

    class UnilocDepthmp : public UniInterfGeometry {

    public:
        void init(const GLuint shader);

    };

    class UnilocWaterry : public UniInterfMesh {

        //////// Vars ////////

    public:
        GLint uDlightProjViewMat[3];

        // Fragment shader

        GLint uViewPos;
        GLint uBaseAmbient;
        GLint uDlightCount;
        GLint uPlightCount;

        GLint u_bansaTex;
        GLint u_gooljulTex;
        GLint u_dudvMap;
        GLint u_normalMap;
        GLint u_dudvMoveFactor;

        GLint uShininess;
        GLint uSpecularStrength;

        GLint uDiffuseColor;
        GLint uHasDiffuseMap;
        GLint uDiffuseMap;

        GLint uDlightDirecs[3];
        GLint uDlightColors[3];
        GLint uDlightDepthMap[3];

        GLint uPlightPoses[3];
        GLint uPlightColors[3];
        GLint uPlightMaxDists[3];

        //////// Funcs ////////

    public:
        void init(const GLuint shader);

    };

    class UnilocAnimate : public UnilocGeneral {

    public:
        GLint i_jointIDs;
        GLint i_weights;

        GLint u_poses[30];

        //////// Funcs ////////

    public:
        void init(const GLuint shader);

    };

}