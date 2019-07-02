#include "p_uniloc.h"

#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include "p_dalopengl.h"
#include "s_logger_god.h"


using namespace std::string_literals;
using namespace fmt::literals;


namespace {

    GLint getUniloc(const GLuint shader, const char* const id, const bool asserting = true) {
        const auto tmp = glGetUniformLocation(shader, id);
        if ( asserting && -1 == tmp ) {
            dalAbort("Failed to find uniloc \'{}\' for shader {}."_format(id, shader));
        }

        return tmp;
    }

    void sendMatrix(const GLint loc, const glm::mat4& mat) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
    }

}


namespace {

    class TextureUnitRegistry {

    private:
        std::unordered_map<std::string, unsigned int> m_reg;
        unsigned int m_size = 0;

    public:
        unsigned int operator[](const std::string& key) {
            const auto iter = this->m_reg.find(key);

            if ( this->m_reg.end() != iter ) {
                return iter->second;
            }
            else {
                const auto result = this->m_reg.emplace(key, this->m_size);
                ++this->m_size;
                return result.second;
            }
        }

    } g_texUnitReg;

}


// SamplerInterf
namespace dal {

    void SamplerInterf::init(const GLuint shader, const GLint sampler, const GLint flagHas) {

    }

    void SamplerInterf::init(const GLuint shader, const GLint sampler) {

    }

    GLint SamplerInterf::getSamplerLoc(void) const {
        return this->m_samplerLoc;
    }

    void SamplerInterf::setFlagHas(const bool x) const {
        glUniform1i(this->m_flagHas, x ? 1 : 0);
    }

    int SamplerInterf::getUnitIndex(void) const {
        return this->m_unitIndex;
    }

}

// UniInterfGeometry
namespace dal {

    void UniInterfGeometry::init(const GLuint shader) {
        dalAssertm(0 == glGetAttribLocation(shader, "i_position"), "Uniloc i_position not found");

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

    void UniInterfMesh::init(const GLuint shader) {
        this->UniInterfGeometry::init(shader);

        dalAssertm(1 == glGetAttribLocation(shader, "i_texCoord"), "Uniloc i_texCoord not found");
        //dalAssertm(2 == glGetAttribLocation(shader, "i_normal"), "Uniloc i_normal not found");

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

    void UniInterfLightedMesh::init(const GLuint shader) {
        this->UniInterfMesh::init(shader);

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

    void UniInterfLightedMesh::dlightDirec(const unsigned int index, const float x, const float y, const float z) const {
        glUniform3f(this->uDlightDirecs[index], x, y, z);
    }

    void UniInterfLightedMesh::dlightDirec(const unsigned int index, const glm::vec3& v) const {
        this->dlightDirec(index, v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::dlightColor(const unsigned int index, const float x, const float y, const float z) const {
        glUniform3f(this->uDlightColors[index], x, y, z);
    }

    void UniInterfLightedMesh::dlightColor(const unsigned int index, const glm::vec3& v) const {
        this->dlightColor(index, v.x, v.y, v.z);
    }

    GLint UniInterfLightedMesh::getDlightDepthMap(const unsigned int index) const {
        return this->uDlightDepthMap[index];
    }

    void UniInterfLightedMesh::dlightProjViewMat(const unsigned int index, glm::mat4& mat) const {
        sendMatrix(this->uDlightProjViewMat[index], mat);
    }

    void UniInterfLightedMesh::plightPos(const unsigned int index, const float x, const float y, const float z) const {
        glUniform3f(this->uPlightPoses[index], x, y, z);
    }

    void UniInterfLightedMesh::plightPos(const unsigned int index, const glm::vec3& v) const {
        this->plightPos(index, v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::plightColor(const unsigned int index, const float x, const float y, const float z) const {
        glUniform3f(this->uPlightColors[index], x, y, z);
    }

    void UniInterfLightedMesh::plightColor(const unsigned int index, const glm::vec3& v) const {
        this->plightColor(index, v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::plightMaxDist(const unsigned int index, const float x) const {
        glUniform1f(this->uPlightMaxDists[index], x);
    }

}


// UniInterfAnime
namespace dal {

    void UniInterfAnime::init(const GLuint shader) {
        dalAssertm(3 == glGetAttribLocation(shader, "i_jointIDs"), "Uniloc i_jointIDs not found");
        dalAssertm(4 == glGetAttribLocation(shader, "i_weights"), "Uniloc i_weights not found");

        for ( unsigned int i = 0; i < this->k_maxNumJoints; ++i ) {
            const auto id = "u_jointTransforms["s + std::to_string(i) + ']';
            this->u_jointTransforms[i] = getUniloc(shader, id.c_str());
        }
    }

    void UniInterfAnime::jointTransforms(const unsigned int index, const glm::mat4& mat) const {
        sendMatrix(this->u_jointTransforms[index], mat);
    }

}


// UniInterfPlaneClip
namespace dal {

    void UniInterfPlaneClip::init(const GLuint shader) {
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


namespace dal {

    void UnilocGeneral::init(const GLuint shader) {
        this->UniInterfLightedMesh::init(shader);
        this->UniInterfPlaneClip::init(shader);

        this->u_diffuseMap = getUniloc(shader, "u_diffuseMap");
    }

    GLint UnilocGeneral::getDiffuseMapLoc(void) const {
        return this->u_diffuseMap;
    }

}


namespace dal {

    

    void UnilocOverlay::init(const GLuint shader) {
        uPoint1 = glGetUniformLocation(shader, "uPoint1");
        uPoint2 = glGetUniformLocation(shader, "uPoint2");

        mUpsideDown_maskMap = glGetUniformLocation(shader, "mUpsideDown_maskMap");
        m_upsideDown_diffuseMap = glGetUniformLocation(shader, "m_upsideDown_diffuseMap");

        // Fragment shader

        uColor = glGetUniformLocation(shader, "uColor");

        mDiffuseMap = glGetUniformLocation(shader, "mDiffuseMap");
        mHasDiffuseMap = glGetUniformLocation(shader, "mHasDiffuseMap");

        mMaskMap = glGetUniformLocation(shader, "mMaskMap");
        mHasMaskMap = glGetUniformLocation(shader, "mHasMaskMap");
    }

    void UnilocFScreen::init(const GLuint shader) {
        iPosition = glGetAttribLocation(shader, "iPosition"); assert(iPosition == 0);
        iTexCoord = glGetAttribLocation(shader, "iTexCoord"); assert(iTexCoord == 1);

        // Fragment shader

        uTexture = glGetUniformLocation(shader, "uTexture");
    }

    void UnilocDepthmp::init(const GLuint shader) {
        this->UniInterfGeometry::init(shader);
    }

    void UnilocWaterry::init(const GLuint shader) {
        this->UniInterfLightedMesh::init(shader);

        this->u_bansaTex = glGetUniformLocation(shader, "u_bansaTex");
        this->u_gooljulTex = glGetUniformLocation(shader, "u_gooljulTex");
        this->u_dudvMap = glGetUniformLocation(shader, "u_dudvMap");
        this->u_normalMap = glGetUniformLocation(shader, "u_normalMap");
        this->u_dudvMoveFactor = glGetUniformLocation(shader, "u_dudvMoveFactor");
    }

    void UnilocAnimate::init(const GLuint shader) {
        this->UnilocGeneral::init(shader);
        this->UniInterfAnime::init(shader);
    }

}