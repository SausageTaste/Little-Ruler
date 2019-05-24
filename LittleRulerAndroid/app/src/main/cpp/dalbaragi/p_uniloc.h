#pragma once

#include "p_dalopengl.h"


namespace dal {

	class UnilocGeneral {

		//////// Vars ////////

	public:
		GLint iPosition;
		GLint iTexCoord;
		GLint iNormal;

		GLint uProjectMat;
		GLint uViewMat;
		GLint uModelMat;

		GLint u_doClip;
		GLint u_clipPlane;

		GLint uDlightProjViewMat[3];

		GLint uTexScaleX;
		GLint uTexScaleY;

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

	class UnilocDepthmp {

		//////// Vars ////////

	public:
		GLint iPosition;

		// Fragment shader

		GLint uProjViewMat;
		GLint uModelMat;

		//////// Funcs ////////

	public:
		void init(const GLuint shader);

	};

	class UnilocWaterry {

		//////// Vars ////////

	public:
		GLint iPosition;
		GLint iTexCoord;
		GLint iNormal;

		GLint uProjectMat;
		GLint uViewMat;
		GLint uModelMat;

		GLint uDlightProjViewMat[3];

		GLint uTexScaleX;
		GLint uTexScaleY;

		// Fragment shader

		GLint uViewPos;
		GLint uBaseAmbient;
		GLint uDlightCount;
		GLint uPlightCount;

		GLint u_bansaTex;
		GLint u_gooljulTex;
		GLint u_dudvMap;
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

}