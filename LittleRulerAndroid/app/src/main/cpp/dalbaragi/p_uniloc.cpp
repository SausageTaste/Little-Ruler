#include "p_uniloc.h"

#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include "p_dalopengl.h"
#include "s_logger_god.h"


#define ASSERT_UNILOC 0


using namespace fmt::literals;


namespace {

    GLint getUniloc(const GLuint shader, const char* const id, const bool asserting = true) {
        const auto tmp = glGetUniformLocation(shader, id);
#if ASSERT_UNILOC != 0
        if ( asserting && -1 == tmp ) {
            dalAbort("Failed to find uniloc \'{}\' for shader {}."_format(id, shader));
        }
#endif
        return tmp;
    }

    inline void sendMatrix(const GLint loc, const glm::mat4& mat) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
    }

    inline void sendBool(const GLint loc, const bool x) {
        glUniform1i(loc, x ? 1 : 0);
    }

}


namespace {

    class TextureUnitRegistry {

    private:
        std::unordered_map<std::string, unsigned int> m_reg;
        unsigned int m_size = 0;

    public:
        unsigned int operator[](const std::string& key) {
            const auto found = this->m_reg.find(key);

            if ( this->m_reg.end() != found ) {
                return found->second;
            }
            else {
                const auto [iter, success] = this->m_reg.emplace(key, this->m_size);
                ++this->m_size;
                return iter->second;
            }
        }

    } g_texUnitReg;

}


// SamplerInterf
namespace dal {

    SamplerInterf::SamplerInterf(const GLint samplerLoc, const GLint flagHasLoc, const unsigned int unitIndex, const bool flagAssert) {
        this->init(samplerLoc, flagHasLoc, unitIndex, flagAssert);
    }

    void SamplerInterf::init(const GLint samplerLoc, const GLint flagHasLoc, const unsigned int unitIndex, const bool flagAssert) {
        this->m_samplerLoc = samplerLoc;
        this->m_flagHas = flagHasLoc;
        this->m_unitIndex = unitIndex;

#if ASSERT_UNILOC != 0
        if ( flagAssert ) {
            if ( -1 == this->m_samplerLoc ) {
                dalAbort("Sampler uniform location is -1.");
            }
            if ( -1 == this->m_flagHas ) {
                dalAbort("Uniform location for flag has is -1. If you meant it, pass in any minus value other than -1, such as -2.");
            }
        }
#endif

        if ( this->m_flagHas < -1 ) {
            this->m_flagHas = -1;
        }
    }

    GLint SamplerInterf::getSamplerLoc(void) const {
        return this->m_samplerLoc;
    }

    void SamplerInterf::setFlagHas(const bool x) const {
        if ( -1 != this->m_flagHas ) {
            glUniform1i(this->m_flagHas, x ? 1 : 0);
        }
    }

    int SamplerInterf::getUnitIndex(void) const {
        return this->m_unitIndex;
    }

}

// UniInterfGeometry
namespace dal {

    UniInterfGeometry::UniInterfGeometry(const GLuint shader) {
#if ASSERT_UNILOC != 0
        dalAssertm(0 == glGetAttribLocation(shader, "i_position"), "Uniloc i_position not found");
#endif
        this->u_projMat = getUniloc(shader, "u_projMat");
        this->u_viewMat = getUniloc(shader, "u_viewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");
    }

    void UniInterfGeometry::projectMat(const glm::mat4& mat) const {
        sendMatrix(this->u_projMat, mat);
    }

    void UniInterfGeometry::viewMat(const glm::mat4& mat) const {
        sendMatrix(this->u_viewMat, mat);
    }

    void UniInterfGeometry::modelMat(const glm::mat4& mat) const {
        sendMatrix(this->u_modelMat, mat);
    }

}


// UniInterfMesh
namespace dal {

    UniInterfMesh::UniInterfMesh(const GLuint shader)
        : UniInterfGeometry(shader)
    {
#if ASSERT_UNILOC != 0
        dalAssertm(1 == glGetAttribLocation(shader, "i_texCoord"), "Uniloc i_texCoord not found");
        //dalAssertm(2 == glGetAttribLocation(shader, "i_normal"), "Uniloc i_normal not found");
#endif

        this->u_texScale = getUniloc(shader, "u_texScale");
    }

    void UniInterfMesh::texScale(const float x, const float y) const {
        glUniform2f(this->u_texScale, x, y);
    }

    void UniInterfMesh::texScale(const glm::vec2& v) const {
        this->texScale(v.x, v.y);
    }

}


// UniInterfLightedMesh
namespace dal {

    UniInterfLightedMesh::UniInterfLightedMesh(const GLuint shader)
        : UniInterfMesh(shader)
    {
        this->uDlightProjViewMat[0] = glGetUniformLocation(shader, "uDlightProjViewMat[0]");
        this->uDlightProjViewMat[1] = glGetUniformLocation(shader, "uDlightProjViewMat[1]");
        this->uDlightProjViewMat[2] = glGetUniformLocation(shader, "uDlightProjViewMat[2]");

        // Fragment shader

        this->uViewPos = glGetUniformLocation(shader, "uViewPos");
        this->uBaseAmbient = glGetUniformLocation(shader, "uBaseAmbient");
        this->uDlightCount = glGetUniformLocation(shader, "uDlightCount");
        this->uPlightCount = glGetUniformLocation(shader, "uPlightCount");

        this->uShininess = glGetUniformLocation(shader, "uShininess");
        this->uSpecularStrength = glGetUniformLocation(shader, "uSpecularStrength");
        this->u_fogMaxPointInvSqr = glGetUniformLocation(shader, "u_fogMaxPointInvSqr");
        this->u_fogColor = glGetUniformLocation(shader, "u_fogColor");

        // Directional Lights

        this->uDlightDirecs[0] = glGetUniformLocation(shader, "uDlightDirecs[0]");
        this->uDlightDirecs[1] = glGetUniformLocation(shader, "uDlightDirecs[1]");
        this->uDlightDirecs[2] = glGetUniformLocation(shader, "uDlightDirecs[2]");

        this->uDlightColors[0] = glGetUniformLocation(shader, "uDlightColors[0]");
        this->uDlightColors[1] = glGetUniformLocation(shader, "uDlightColors[1]");
        this->uDlightColors[2] = glGetUniformLocation(shader, "uDlightColors[2]");

        for ( int i = 0; i < this->k_maxDlight; ++i ) {
            const auto id = "uDlightDepthMap[{}]"_format(i);
            this->uDlightDepthMap[i].init(getUniloc(shader, id.c_str()), -2, g_texUnitReg[id.c_str()]);
        }

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

    void UniInterfLightedMesh::viewPos(const float x, const float y, const float z) const {
        glUniform3f(this->uViewPos, x, y, z);
    }

    void UniInterfLightedMesh::viewPos(const glm::vec3& v) const {
        this->viewPos(v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::baseAmbient(const float x, const float y, const float z) const {
        glUniform3f(this->uBaseAmbient, x, y, z);
    }

    void UniInterfLightedMesh::baseAmbient(const glm::vec3& v) const {
        this->baseAmbient(v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::dlightCount(const unsigned int x) const {
        glUniform1i(this->uDlightCount, x);
    }

    void UniInterfLightedMesh::plightCount(const unsigned int x) const {
        glUniform1i(this->uPlightCount, x);
    }

    void UniInterfLightedMesh::shininess(const float x) const {
        glUniform1f(this->uShininess, x);
    }

    void UniInterfLightedMesh::specularStrength(const float x) const {
        glUniform1f(this->uSpecularStrength, x);
    }

    void UniInterfLightedMesh::fogMaxPoint(const float x) const {
        dalAssert(0 != x);
        glUniform1f(this->u_fogMaxPointInvSqr, 1.0f / (x * x));
    }

    void UniInterfLightedMesh::fogMaxPointAsInfinity(void) const {
        glUniform1f(this->u_fogMaxPointInvSqr, 0.0f);
    }

    void UniInterfLightedMesh::fogColor(const float x, const float y, const float z) const {
        glUniform3f(this->u_fogColor, x, y, z);
    }

    void UniInterfLightedMesh::fogColor(const glm::vec3& v) const {
        this->fogColor(v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::dlightDirec(const unsigned int index, const float x, const float y, const float z) const {
        dalAssert(index < this->k_maxDlight);
        glUniform3f(this->uDlightDirecs[index], x, y, z);
    }

    void UniInterfLightedMesh::dlightDirec(const unsigned int index, const glm::vec3& v) const {
        dalAssert(index < this->k_maxDlight);
        this->dlightDirec(index, v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::dlightColor(const unsigned int index, const float x, const float y, const float z) const {
        dalAssert(index < this->k_maxDlight);
        glUniform3f(this->uDlightColors[index], x, y, z);
    }

    void UniInterfLightedMesh::dlightColor(const unsigned int index, const glm::vec3& v) const {
        dalAssert(index < this->k_maxDlight);
        this->dlightColor(index, v.x, v.y, v.z);
    }

    const SamplerInterf& UniInterfLightedMesh::getDlightDepthMap(const unsigned int index) const {
        dalAssert(index < this->k_maxDlight);
        return this->uDlightDepthMap[index];
    }

    void UniInterfLightedMesh::dlightProjViewMat(const unsigned int index, glm::mat4& mat) const {
        dalAssert(index < this->k_maxDlight);
        sendMatrix(this->uDlightProjViewMat[index], mat);
    }

    void UniInterfLightedMesh::plightPos(const unsigned int index, const float x, const float y, const float z) const {
        dalAssert(index < this->k_maxPlight);
        glUniform3f(this->uPlightPoses[index], x, y, z);
    }

    void UniInterfLightedMesh::plightPos(const unsigned int index, const glm::vec3& v) const {
        dalAssert(index < this->k_maxPlight);
        this->plightPos(index, v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::plightColor(const unsigned int index, const float x, const float y, const float z) const {
        dalAssert(index < this->k_maxPlight);
        glUniform3f(this->uPlightColors[index], x, y, z);
    }

    void UniInterfLightedMesh::plightColor(const unsigned int index, const glm::vec3& v) const {
        dalAssert(index < this->k_maxPlight);
        this->plightColor(index, v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::plightMaxDist(const unsigned int index, const float x) const {
        dalAssert(index < this->k_maxPlight);
        glUniform1f(this->uPlightMaxDists[index], x);
    }

}


// UniInterfAnime
namespace dal {

    UniInterfAnime::UniInterfAnime(const GLuint shader) {
#if ASSERT_UNILOC != 0
        dalAssertm(3 == glGetAttribLocation(shader, "i_jointIDs"), "Uniloc i_jointIDs not found");
        dalAssertm(4 == glGetAttribLocation(shader, "i_weights"), "Uniloc i_weights not found");
#endif

        for ( unsigned int i = 0; i < this->k_maxNumJoints; ++i ) {
            const auto id = "u_jointTransforms[{}]"_format(i);
            this->u_jointTransforms[i] = getUniloc(shader, id.c_str());
        }
    }

    void UniInterfAnime::jointTransforms(const unsigned int index, const glm::mat4& mat) const {
        dalAssert(index < this->k_maxNumJoints);
        sendMatrix(this->u_jointTransforms[index], mat);
    }

}


// UniInterfPlaneClip
namespace dal {

    UniInterfPlaneClip::UniInterfPlaneClip(const GLuint shader) {
        this->u_doClip = getUniloc(shader, "u_doClip");
        this->u_clipPlane = getUniloc(shader, "u_clipPlane");
    }

    void UniInterfPlaneClip::flagDoClip(const bool x) const {
        glUniform1i(this->u_doClip, x ? 1 : 0);
    }

    void UniInterfPlaneClip::clipPlane(const glm::vec4& plane) const {
        this->clipPlane(plane.x, plane.y, plane.z, plane.w);
    }

    void UniInterfPlaneClip::clipPlane(const float x, const float y, const float z, const float w) const {
        glUniform4f(this->u_clipPlane, x, y, z, w);
    }

}


// UnilocGeneral
namespace dal {

    UnilocGeneral::UnilocGeneral(const GLuint shader)
        : m_lightedMesh(shader)
        , m_planeClip(shader)
    {
        this->u_diffuseMap.init(getUniloc(shader, "u_diffuseMap"), -2, g_texUnitReg["u_diffuseMap"]);
    }

    const SamplerInterf& UnilocGeneral::getDiffuseMapLoc(void) const {
        return this->u_diffuseMap;
    }

}


// UnilocOverlay
namespace dal {

    UnilocOverlay::UnilocOverlay(const GLuint shader) {
        this->uPoint1 = getUniloc(shader, "uPoint1");
        this->uPoint2 = getUniloc(shader, "uPoint2");

        this->mUpsideDown_maskMap = getUniloc(shader, "mUpsideDown_maskMap");
        this->m_upsideDown_diffuseMap = getUniloc(shader, "m_upsideDown_diffuseMap");

        this->u_texOffset = getUniloc(shader, "u_texOffset");
        this->u_texScale = getUniloc(shader, "u_texScale");

        // Fragment shader

        this->uColor = getUniloc(shader, "uColor");

        this->m_diffuseMap.init(getUniloc(shader, "mDiffuseMap"), getUniloc(shader, "mHasDiffuseMap"), g_texUnitReg["mDiffuseMap"]);
        this->m_maskMap.init(getUniloc(shader, "mMaskMap"), getUniloc(shader, "mHasMaskMap"), g_texUnitReg["mMaskMap"]);
    }

    void UnilocOverlay::texOffset(const float x, const float y) const {
        glUniform2f(this->u_texOffset, x, y);
    }

    void UnilocOverlay::texOffset(const glm::vec2& v) const {
        this->texOffset(v.x, v.y);
    }

    void UnilocOverlay::texScale(const float x, const float y) const {
        glUniform2f(this->u_texScale, x, y);
    }

    void UnilocOverlay::texScale(const glm::vec2& v) const {
        this->texScale(v.x, v.y);
    }

    void UnilocOverlay::point1(const glm::vec2& v) const {
        glUniform2f(this->uPoint1, v.x, v.y);
    }

    void UnilocOverlay::point2(const glm::vec2& v) const {
        glUniform2f(this->uPoint2, v.x, v.y);
    }

    void UnilocOverlay::upsideDownDiffuseMap(const bool x) const {
        sendBool(this->m_upsideDown_diffuseMap, x);
    }

    void UnilocOverlay::upsideDownMaskMap(const bool x) const {
        sendBool(this->mUpsideDown_maskMap, x);
    }

    void UnilocOverlay::color(const glm::vec4& v) const {
        glUniform4f(this->uColor, v.x, v.y, v.z, v.w);
    }

    const SamplerInterf& UnilocOverlay::getDiffuseMap(void) const {
        return this->m_diffuseMap;
    }

    const SamplerInterf& UnilocOverlay::getMaskMap(void) const {
        return this->m_maskMap;
    }

}


// UnilocFScreen
namespace dal {

    UnilocFScreen::UnilocFScreen(const GLuint shader) {
#if ASSERT_UNILOC != 0
        dalAssert(0 == glGetAttribLocation(shader, "iPosition"));
        dalAssert(1 == glGetAttribLocation(shader, "iTexCoord"));
#endif

        this->m_texture.init(getUniloc(shader, "uTexture"), -2, g_texUnitReg["uTexture"]);
    }

    const SamplerInterf& UnilocFScreen::getTexture(void) const {
        return this->m_texture;
    }

}


// UnilocWaterry
namespace dal {

    UnilocWaterry::UnilocWaterry(const GLuint shader)
        : m_lightedMesh(shader)
    {
        this->u_dudvMoveFactor = getUniloc(shader, "u_dudvMoveFactor");
        this->u_waveStrength = getUniloc(shader, "u_waveStrength");
        this->u_darkestDepthPoint = getUniloc(shader, "u_darkestDepthPoint");
        this->u_reflectivity = getUniloc(shader, "u_reflectivity");
        this->u_deepColor = getUniloc(shader, "u_deepColor");

        this->m_bansaTex.init(getUniloc(shader, "u_bansaTex"), -2, g_texUnitReg["u_bansaTex"]);
        this->m_gooljulTex.init(getUniloc(shader, "u_gooljulTex"), -2, g_texUnitReg["u_gooljulTex"]);
        this->m_dudvMap.init(getUniloc(shader, "u_dudvMap"), -2, g_texUnitReg["u_dudvMap"]);
        this->m_normalMap.init(getUniloc(shader, "u_normalMap"), -2, g_texUnitReg["u_normalMap"]);
        this->m_depthMap.init(getUniloc(shader, "u_depthMap"), -2, g_texUnitReg["u_depthMap"]);
    }

    void UnilocWaterry::dudvFactor(const float x) const {
        glUniform1f(this->u_dudvMoveFactor, x);
    }

    void UnilocWaterry::waveStrength(const float x) const {
        glUniform1f(this->u_waveStrength, x);
    }

    void UnilocWaterry::darkestDepthPoint(const float x) const {
        glUniform1f(this->u_darkestDepthPoint, x);
    }

    void UnilocWaterry::reflectivity(const float x) const {
        glUniform1f(this->u_reflectivity, x);
    }

    void UnilocWaterry::deepColor(const float x, const float y, const float z) const {
        glUniform3f(this->u_deepColor, x, y, z);
    }

    void UnilocWaterry::deepColor(const glm::vec3& v) const {
        this->deepColor(v.x, v.y, v.z);
    }

    const SamplerInterf& UnilocWaterry::getReflectionTex(void) const {
        return this->m_bansaTex;
    }

    const SamplerInterf& UnilocWaterry::getRefractionTex(void) const {
        return this->m_gooljulTex;
    }

    const SamplerInterf& UnilocWaterry::getDUDVMap(void) const {
        return this->m_dudvMap;
    }

    const SamplerInterf& UnilocWaterry::getNormalMap(void) const {
        return this->m_normalMap;
    }

    const SamplerInterf& UnilocWaterry::getDepthMap(void) const {
        return this->m_depthMap;
    }

}


// UnilocDepthAnime
namespace dal {

    UnilocDepthAnime::UnilocDepthAnime(const GLuint shader)
        : m_geometry(shader)
        , m_anime(shader)
    {

    }

}


// UnilocSkybox
namespace dal {

    UnilocSkybox::UnilocSkybox(const GLuint shader)
        : m_geometry(shader) 
    {
        this->u_skyboxTex.init(getUniloc(shader, "u_skyboxTex"), -2, g_texUnitReg["u_skyboxTex"]);
    }

    const SamplerInterf& UnilocSkybox::getSkyboxTexLoc(void) const {
        return this->u_skyboxTex;
    }

}


namespace dal {

    UnilocDepthmp::UnilocDepthmp(const GLuint shader)
        : m_geometry(shader)
    {

    }

    UnilocAnimate::UnilocAnimate(const GLuint shader)
        : UnilocGeneral(shader)
        , m_anime(shader)
    {

    }

}