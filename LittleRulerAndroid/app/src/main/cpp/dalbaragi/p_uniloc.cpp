#include "p_uniloc.h"

#include <string>
#include <cassert>

#include "p_dalopengl.h"
#include "s_logger_god.h"


using namespace std::string_literals;


namespace dal {

	void UnilocGeneral::init(const GLuint shader) {
		this->iPosition = glGetAttribLocation(shader, "iPosition"); assert(this->iPosition == 0);
		this->iTexCoord = glGetAttribLocation(shader, "iTexCoord"); assert(this->iTexCoord == 1);
		this->iNormal = glGetAttribLocation(shader, "iNormal"); assert(this->iNormal == 2);

		this->uProjectMat = glGetUniformLocation(shader, "uProjectMat");
		this->uViewMat = glGetUniformLocation(shader, "uViewMat");
		this->uModelMat = glGetUniformLocation(shader, "uModelMat");

		this->u_doClip = glGetUniformLocation(shader, "u_doClip");
		this->u_clipPlane = glGetUniformLocation(shader, "u_clipPlane");

		this->uDlightProjViewMat[0] = glGetUniformLocation(shader, "uDlightProjViewMat[0]");
		this->uDlightProjViewMat[1] = glGetUniformLocation(shader, "uDlightProjViewMat[1]");
		this->uDlightProjViewMat[2] = glGetUniformLocation(shader, "uDlightProjViewMat[2]");

		this->uTexScaleX = glGetUniformLocation(shader, "uTexScaleX");
		this->uTexScaleY = glGetUniformLocation(shader, "uTexScaleY");

		// Fragment shader

		this->uViewPos = glGetUniformLocation(shader, "uViewPos");
		this->uBaseAmbient = glGetUniformLocation(shader, "uBaseAmbient");
		this->uDlightCount = glGetUniformLocation(shader, "uDlightCount");
		this->uPlightCount = glGetUniformLocation(shader, "uPlightCount");

		this->uShininess = glGetUniformLocation(shader, "uShininess");
		this->uSpecularStrength = glGetUniformLocation(shader, "uSpecularStrength");

		this->uDiffuseColor = glGetUniformLocation(shader, "uDiffuseColor");
		this->uHasDiffuseMap = glGetUniformLocation(shader, "uHasDiffuseMap");
		this->uDiffuseMap = glGetUniformLocation(shader, "uDiffuseMap");

		// Directional Lights

		this->uDlightDirecs[0] = glGetUniformLocation(shader, "uDlightDirecs[0]");
		this->uDlightDirecs[1] = glGetUniformLocation(shader, "uDlightDirecs[1]");
		this->uDlightDirecs[2] = glGetUniformLocation(shader, "uDlightDirecs[2]");

		this->uDlightColors[0] = glGetUniformLocation(shader, "uDlightColors[0]");
		this->uDlightColors[1] = glGetUniformLocation(shader, "uDlightColors[1]");
		this->uDlightColors[2] = glGetUniformLocation(shader, "uDlightColors[2]");

		this->uDlightDepthMap[0] = glGetUniformLocation(shader, "uDlightDepthMap[0]");
		this->uDlightDepthMap[1] = glGetUniformLocation(shader, "uDlightDepthMap[1]");
		this->uDlightDepthMap[2] = glGetUniformLocation(shader, "uDlightDepthMap[2]");

		// Point Lights

		this->uPlightPoses[0] = glGetUniformLocation(shader, "uPlightPoses[0]");
		this->uPlightPoses[1] = glGetUniformLocation(shader, "uPlightPoses[1]");
		this->uPlightPoses[2] = glGetUniformLocation(shader, "uPlightPoses[2]");

		this->uPlightColors[0] = glGetUniformLocation(shader, "uPlightColors[0]");
		this->uPlightColors[1] = glGetUniformLocation(shader, "uPlightColors[1]");
		this->uPlightColors[2] = glGetUniformLocation(shader, "uPlightColors[2]");

		this->uPlightMaxDists[0] = glGetUniformLocation(shader, "uPlightMaxDists[0]");
		this->uPlightMaxDists[1] = glGetUniformLocation(shader, "uPlightMaxDists[1]");
		this->uPlightMaxDists[2] = glGetUniformLocation(shader, "uPlightMaxDists[2]");
	}

}


namespace dal {

	void UnilocOverlay::init(const GLuint shader) {
		uPoint1 = glGetUniformLocation(shader, "uPoint1");
		uPoint2 = glGetUniformLocation(shader, "uPoint2");

		mUpsideDown_maskMap = glGetUniformLocation(shader, "mUpsideDown_maskMap");

		// Fragment shader

		uColor = glGetUniformLocation(shader, "uColor");

		mDiffuseMap = glGetUniformLocation(shader, "mDiffuseMap");
		mHasDiffuseMap = glGetUniformLocation(shader, "mHasDiffuseMap");

		mMaskMap = glGetUniformLocation(shader, "mMaskMap");
		mHasMaskMap = glGetUniformLocation(shader, "mHasMaskMap");
	}

}


namespace dal {

	void UnilocFScreen::init(const GLuint shader) {
		iPosition = glGetAttribLocation(shader, "iPosition"); assert(iPosition == 0);
		iTexCoord = glGetAttribLocation(shader, "iTexCoord"); assert(iTexCoord == 1);

		// Fragment shader

		uTexture = glGetUniformLocation(shader, "uTexture");
	}

}


namespace dal {

	void UnilocDepthmp::init(const GLuint shader) {
		iPosition = glGetAttribLocation(shader, "iPosition"); assert(iPosition == 0);

		// Fragment shader

		uProjViewMat = glGetUniformLocation(shader, "uProjViewMat");
		uModelMat = glGetUniformLocation(shader, "uModelMat");
	}

}


namespace dal {

	void UnilocWaterry::init(const GLuint shader) {
		this->iPosition = glGetAttribLocation(shader, "iPosition"); assert(this->iPosition == 0);
		this->iTexCoord = glGetAttribLocation(shader, "iTexCoord");
		this->iNormal = glGetAttribLocation(shader, "iNormal");

		this->uProjectMat = glGetUniformLocation(shader, "uProjectMat");
		this->uViewMat = glGetUniformLocation(shader, "uViewMat");
		this->uModelMat = glGetUniformLocation(shader, "uModelMat");

		this->uDlightProjViewMat[0] = glGetUniformLocation(shader, "uDlightProjViewMat[0]");
		this->uDlightProjViewMat[1] = glGetUniformLocation(shader, "uDlightProjViewMat[1]");
		this->uDlightProjViewMat[2] = glGetUniformLocation(shader, "uDlightProjViewMat[2]");

		this->uTexScaleX = glGetUniformLocation(shader, "uTexScaleX");
		this->uTexScaleY = glGetUniformLocation(shader, "uTexScaleY");

		// Fragment shader

		this->uViewPos = glGetUniformLocation(shader, "uViewPos");
		this->uBaseAmbient = glGetUniformLocation(shader, "uBaseAmbient");
		this->uDlightCount = glGetUniformLocation(shader, "uDlightCount");
		this->uPlightCount = glGetUniformLocation(shader, "uPlightCount");

		this->u_bansaTex = glGetUniformLocation(shader, "u_bansaTex");
		this->u_gooljulTex = glGetUniformLocation(shader, "u_gooljulTex");
		this->u_dudvMap = glGetUniformLocation(shader, "u_dudvMap");
		this->u_dudvMoveFactor = glGetUniformLocation(shader, "u_dudvMoveFactor");

		this->uShininess = glGetUniformLocation(shader, "uShininess");
		this->uSpecularStrength = glGetUniformLocation(shader, "uSpecularStrength");

		this->uDiffuseColor = glGetUniformLocation(shader, "uDiffuseColor");
		this->uHasDiffuseMap = glGetUniformLocation(shader, "uHasDiffuseMap");
		this->uDiffuseMap = glGetUniformLocation(shader, "uDiffuseMap");

		// Directional Lights

		this->uDlightDirecs[0] = glGetUniformLocation(shader, "uDlightDirecs[0]");
		this->uDlightDirecs[1] = glGetUniformLocation(shader, "uDlightDirecs[1]");
		this->uDlightDirecs[2] = glGetUniformLocation(shader, "uDlightDirecs[2]");

		this->uDlightColors[0] = glGetUniformLocation(shader, "uDlightColors[0]");
		this->uDlightColors[1] = glGetUniformLocation(shader, "uDlightColors[1]");
		this->uDlightColors[2] = glGetUniformLocation(shader, "uDlightColors[2]");

		this->uDlightDepthMap[0] = glGetUniformLocation(shader, "uDlightDepthMap[0]");
		this->uDlightDepthMap[1] = glGetUniformLocation(shader, "uDlightDepthMap[1]");
		this->uDlightDepthMap[2] = glGetUniformLocation(shader, "uDlightDepthMap[2]");

		// Point Lights

		this->uPlightPoses[0] = glGetUniformLocation(shader, "uPlightPoses[0]");
		this->uPlightPoses[1] = glGetUniformLocation(shader, "uPlightPoses[1]");
		this->uPlightPoses[2] = glGetUniformLocation(shader, "uPlightPoses[2]");

		this->uPlightColors[0] = glGetUniformLocation(shader, "uPlightColors[0]");
		this->uPlightColors[1] = glGetUniformLocation(shader, "uPlightColors[1]");
		this->uPlightColors[2] = glGetUniformLocation(shader, "uPlightColors[2]");

		this->uPlightMaxDists[0] = glGetUniformLocation(shader, "uPlightMaxDists[0]");
		this->uPlightMaxDists[1] = glGetUniformLocation(shader, "uPlightMaxDists[1]");
		this->uPlightMaxDists[2] = glGetUniformLocation(shader, "uPlightMaxDists[2]");
	}

}