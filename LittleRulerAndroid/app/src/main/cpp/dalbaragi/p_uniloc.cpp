#include "p_uniloc.h"

#include <string>

#include "s_logger_god.h"


using namespace std;


namespace dal {

	UnilocGeneral::UnilocGeneral(void) {

	}

	void UnilocGeneral::init(const ShaderProgram& shader) {
		this->iPosition = shader.getAttribLocation("iPosition");
		this->iTexCoord = shader.getAttribLocation("iTexCoord");
		this->iNormal = shader.getAttribLocation("iNormal");

		this->uProjectMat = shader.getUniformLocation("uProjectMat");
		this->uViewMat = shader.getUniformLocation("uViewMat");
		this->uModelMat = shader.getUniformLocation("uModelMat");

		this->uDlightProjViewMat[0] = shader.getUniformLocation("uDlightProjViewMat[0]");
		this->uDlightProjViewMat[1] = shader.getUniformLocation("uDlightProjViewMat[1]");
		this->uDlightProjViewMat[2] = shader.getUniformLocation("uDlightProjViewMat[2]");

		this->uTexScaleX = shader.getUniformLocation("uTexScaleX");
		this->uTexScaleY = shader.getUniformLocation("uTexScaleY");

		// Fragment shader

		this->uViewPos = shader.getUniformLocation("uViewPos");
		this->uBaseAmbient = shader.getUniformLocation("uBaseAmbient");
		this->uDlightCount = shader.getUniformLocation("uDlightCount");
		this->uPlightCount = shader.getUniformLocation("uPlightCount");

		this->uShininess = shader.getUniformLocation("uShininess");
		this->uSpecularStrength = shader.getUniformLocation("uSpecularStrength");

		this->uDiffuseColor = shader.getUniformLocation("uDiffuseColor");
		this->uHasDiffuseMap = shader.getUniformLocation("uHasDiffuseMap");
		this->uDiffuseMap = shader.getUniformLocation("uDiffuseMap");

		// Directional Lights

		this->uDlightDirecs[0] = shader.getUniformLocation("uDlightDirecs[0]");
		this->uDlightDirecs[1] = shader.getUniformLocation("uDlightDirecs[1]");
		this->uDlightDirecs[2] = shader.getUniformLocation("uDlightDirecs[2]");

		this->uDlightColors[0] = shader.getUniformLocation("uDlightColors[0]");
		this->uDlightColors[1] = shader.getUniformLocation("uDlightColors[1]");
		this->uDlightColors[2] = shader.getUniformLocation("uDlightColors[2]");

		this->uDlightDepthMap[0] = shader.getUniformLocation("uDlightDepthMap[0]");
		this->uDlightDepthMap[1] = shader.getUniformLocation("uDlightDepthMap[1]");
		this->uDlightDepthMap[2] = shader.getUniformLocation("uDlightDepthMap[2]");

		// Point Lights

		this->uPlightPoses[0] = shader.getUniformLocation("uPlightPoses[0]");
		this->uPlightPoses[1] = shader.getUniformLocation("uPlightPoses[1]");
		this->uPlightPoses[2] = shader.getUniformLocation("uPlightPoses[2]");

		this->uPlightColors[0] = shader.getUniformLocation("uPlightColors[0]");
		this->uPlightColors[1] = shader.getUniformLocation("uPlightColors[1]");
		this->uPlightColors[2] = shader.getUniformLocation("uPlightColors[2]");

		this->uPlightMaxDists[0] = shader.getUniformLocation("uPlightMaxDists[0]");
		this->uPlightMaxDists[1] = shader.getUniformLocation("uPlightMaxDists[1]");
		this->uPlightMaxDists[2] = shader.getUniformLocation("uPlightMaxDists[2]");
	}

}


namespace dal {

	UnilocOverlay::UnilocOverlay(void) {

	}

	void UnilocOverlay::init(const ShaderProgram& shader) {
		uPoint1 = shader.getUniformLocation("uPoint1");
		uPoint2 = shader.getUniformLocation("uPoint2");

		mUpsideDown_maskMap = shader.getUniformLocation("mUpsideDown_maskMap");

		// Fragment shader

		uColor = shader.getUniformLocation("uColor");

		mDiffuseMap = shader.getUniformLocation("mDiffuseMap");
		mHasDiffuseMap = shader.getUniformLocation("mHasDiffuseMap");

		mMaskMap = shader.getUniformLocation("mMaskMap");
		mHasMaskMap = shader.getUniformLocation("mHasMaskMap");
	}

}


namespace dal {

	void UnilocFScreen::init(const ShaderProgram& shader) {
		iPosition = shader.getAttribLocation("iPosition");
		iTexCoord = shader.getAttribLocation("iTexCoord");

		// Fragment shader

		uTexture = shader.getUniformLocation("uTexture");
	}

}


namespace dal {

	void UnilocDepthmp::init(const ShaderProgram& shader) {
		iPosition = shader.getAttribLocation("iPosition");

		// Fragment shader

		uProjViewMat = shader.getUniformLocation("uProjViewMat");
		uModelMat = shader.getUniformLocation("uModelMat");
	}

}