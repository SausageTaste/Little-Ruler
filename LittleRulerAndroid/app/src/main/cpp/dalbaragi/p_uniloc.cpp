#include "p_uniloc.h"

#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include "p_dalopengl.h"
#include "s_logger_god.h"


#define ASSERT_UNILOC false


using namespace fmt::literals;


// Konstants
namespace {

    constexpr size_t MAX_JOINTS_NUM = 30;

}


// Utils
namespace {

    GLint getUniloc(const GLuint shader, const char* const id, const bool asserting = true) {
        const auto tmp = glGetUniformLocation(shader, id);
#if ASSERT_UNILOC
        if ( asserting && -1 == tmp ) {
            dalWarn("Failed to find uniloc \'{}\' for shader {}."_format(id, shader));
        }
#endif
        return tmp;
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

#if ASSERT_UNILOC
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


namespace dal {

    void UniInterf_Envmap::set(const GLuint shader) {
        this->u_envmap.init(getUniloc(shader, "u_envmap"), -2, g_texUnitReg["u_envmap"]);
    }

    void UniInterf_Lighting::set(const GLuint shader) {
        this->u_baseAmbient = getUniloc(shader, "u_baseAmbient");

        this->u_roughness = getUniloc(shader, "u_roughness");
        this->u_metallic = getUniloc(shader, "u_metallic");

        this->u_dlightCount = getUniloc(shader, "u_dlightCount");
        this->u_plightCount = getUniloc(shader, "u_plightCount");
        this->u_slightCount = getUniloc(shader, "u_slightCount");

        this->u_plight_poses = getUniloc(shader, "u_plight_poses");
        this->u_plight_colors = getUniloc(shader, "u_plight_colors");

        this->u_dlight_direcs = getUniloc(shader, "u_dlight_direcs");
        this->u_dlight_colors = getUniloc(shader, "u_dlight_colors");
        this->u_dlight_projViewMat = getUniloc(shader, "u_dlight_projViewMat");
        for ( unsigned i = 0; i < 3; ++i ) {
            const auto id = fmt::format("u_dlight_shadowmap[{}]", i);
            this->u_dlight_shadowmap[i].init(getUniloc(shader, id.c_str()), -2, g_texUnitReg[id]);
        }

        this->u_slight_poses = getUniloc(shader, "u_slight_poses");
        this->u_slight_direcs = getUniloc(shader, "u_slight_direcs");
        this->u_slight_colors = getUniloc(shader, "u_slight_colors");
        this->u_slight_fadeStart = getUniloc(shader, "u_slight_fadeStart");
        this->u_slight_fadeEnd = getUniloc(shader, "u_slight_fadeEnd");
        this->u_slight_projViewMat = getUniloc(shader, "u_slight_projViewMat");
        for ( unsigned i = 0; i < 3; ++i ) {
            const auto id = fmt::format("u_slight_shadowmap[{}]", i);
            u_slight_shadowmap[i].init(getUniloc(shader, id.c_str()), -2, g_texUnitReg[id]);
        }
    }

    void UniInterf_Lightmap::set(const GLuint shader) {
        this->u_diffuseMap.init(getUniloc(shader, "u_diffuseMap"), -2, g_texUnitReg["u_diffuseMap"]);
        this->u_roughnessMap.init(getUniloc(shader, "u_roughnessMap"), getUniloc(shader, "u_hasRoughnessMap"), g_texUnitReg["u_roughnessMap"]);
        this->u_metallicMap.init(getUniloc(shader, "u_metallicMap"), getUniloc(shader, "u_hasMetallicMap"), g_texUnitReg["u_metallicMap"]);
    }

    void UniRender_Static::set(const GLuint shader) {
        this->i_lighting.set(shader);
        this->i_lightmap.set(shader);

        this->u_projMat = getUniloc(shader, "u_projMat");
        this->u_viewMat = getUniloc(shader, "u_viewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");

        this->u_viewPos = getUniloc(shader, "u_viewPos");
    }

}


// UniInterfLightmaps
namespace dal {

    UniInterfLightmaps::UniInterfLightmaps(const GLuint shader) {
        this->u_diffuseMap.init(getUniloc(shader, "u_diffuseMap"), -2, g_texUnitReg["u_diffuseMap"]);
        this->u_roughnessMap.init(getUniloc(shader, "u_roughnessMap"), getUniloc(shader, "u_hasRoughnessMap"), g_texUnitReg["u_roughnessMap"]);
        this->u_metallicMap.init(getUniloc(shader, "u_metallicMap"), getUniloc(shader, "u_hasMetallicMap"), g_texUnitReg["u_metallicMap"]);
    }

}


// UniInterfGeometry
namespace dal {

    UniInterfGeometry::UniInterfGeometry(const GLuint shader) {
#if ASSERT_UNILOC
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
#if ASSERT_UNILOC
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


// SpotLight
namespace dal {

    void UniInterfLightedMesh::SpotLight::init(const GLuint shader, const unsigned int index) {
        this->m_pos = getUniloc(shader, fmt::format("u_slights[{}].m_pos", index).c_str());
        this->m_direc = getUniloc(shader, fmt::format("u_slights[{}].m_direc", index).c_str());
        this->m_color = getUniloc(shader, fmt::format("u_slights[{}].m_color", index).c_str());
        this->m_startFade = getUniloc(shader, fmt::format("u_slights[{}].m_startFade", index).c_str());
        this->m_endFade = getUniloc(shader, fmt::format("u_slights[{}].m_endFade", index).c_str());

        this->u_projViewMat = getUniloc(shader, fmt::format("u_slightProjViewMat[{}]", index).c_str());
        const auto id = "u_slightDepthMap[{}]"_format(index);
        this->m_depthMap.init(getUniloc(shader, id.c_str()), -2, g_texUnitReg[id], false);
    }

    void UniInterfLightedMesh::SpotLight::pos(const float x, const float y, const float z) const {
        glUniform3f(this->m_pos, x, y, z);
    }
    void UniInterfLightedMesh::SpotLight::pos(const glm::vec3& v) const {
        this->pos(v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::SpotLight::direc(const float x, const float y, const float z) const {
        glUniform3f(this->m_direc, x, y, z);
    }
    void UniInterfLightedMesh::SpotLight::direc(const glm::vec3& v) const {
        this->direc(v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::SpotLight::color(const float x, const float y, const float z) const {
        glUniform3f(this->m_color, x, y, z);
    }
    void UniInterfLightedMesh::SpotLight::color(const glm::vec3& v) const {
        this->color(v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::SpotLight::startFade(const float v) const {
        glUniform1f(this->m_startFade, v);
    }
    void UniInterfLightedMesh::SpotLight::endFade(const float v) const {
        glUniform1f(this->m_endFade, v);
    }

    void UniInterfLightedMesh::SpotLight::projViewMat(const glm::mat4& mat) const {
        sendMatrix(this->u_projViewMat, mat);
    }

}


// DirecLight
namespace dal {

    void UniInterfLightedMesh::DirecLight::init(const GLuint shader, const unsigned int index) {
        this->m_direc = getUniloc(shader, fmt::format("u_dlights[{}].m_direc", index).c_str());
        this->m_color = getUniloc(shader, fmt::format("u_dlights[{}].m_color", index).c_str());

        this->u_projViewMat = getUniloc(shader, fmt::format("uDlightProjViewMat[{}]", index).c_str());
        const auto id = "uDlightDepthMap[{}]"_format(index);
        this->m_depthMap.init(getUniloc(shader, id.c_str()), -2, g_texUnitReg[id], false);
    }

    void UniInterfLightedMesh::DirecLight::direc(const float x, const float y, const float z) const {
        glUniform3f(this->m_direc, x, y, z);
    }

    void UniInterfLightedMesh::DirecLight::direc(const glm::vec3& v) const {
        this->direc(v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::DirecLight::color(const float x, const float y, const float z) const {
        glUniform3f(this->m_color, x, y, z);
    }
    void UniInterfLightedMesh::DirecLight::color(const glm::vec3& v) const {
        this->color(v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::DirecLight::projViewMat(const glm::mat4& mat) const {
        sendMatrix(this->u_projViewMat, mat);
    }

}


// PointLight
namespace dal {

    void UniInterfLightedMesh::PointLight::init(const GLuint shader, const unsigned int index) {
        this->m_pos = getUniloc(shader, fmt::format("u_plights[{}].m_pos", index).c_str());
        this->m_color = getUniloc(shader, fmt::format("u_plights[{}].m_color", index).c_str());
    }

    void UniInterfLightedMesh::PointLight::pos(const float x, const float y, const float z) const {
        glUniform3f(this->m_pos, x, y, z);
    }
    void UniInterfLightedMesh::PointLight::pos(const glm::vec3& v) const {
        this->pos(v.x, v.y, v.z);
    }

    void UniInterfLightedMesh::PointLight::color(const float x, const float y, const float z) const {
        glUniform3f(this->m_color, x, y, z);
    }
    void UniInterfLightedMesh::PointLight::color(const glm::vec3& v) const {
        this->color(v.x, v.y, v.z);
    }

}


// UniInterfLightedMesh
namespace dal {

    UniInterfLightedMesh::UniInterfLightedMesh(const GLuint shader)
        : UniInterfMesh(shader)
    {
        // Fragment shader

        this->uViewPos = glGetUniformLocation(shader, "uViewPos");
        this->uBaseAmbient = glGetUniformLocation(shader, "uBaseAmbient");
        this->uDlightCount = glGetUniformLocation(shader, "uDlightCount");
        this->uPlightCount = glGetUniformLocation(shader, "uPlightCount");
        this->u_slightCount = glGetUniformLocation(shader, "u_slightCount");

        this->u_roughness = getUniloc(shader, "u_roughness");
        this->u_metallic = getUniloc(shader, "u_metallic");

        this->u_fogMaxPointInvSqr = glGetUniformLocation(shader, "u_fogMaxPointInvSqr");
        this->u_fogColor = glGetUniformLocation(shader, "u_fogColor");

        for ( unsigned int i = 0; i < this->k_maxSlight; ++i ) {
            this->u_slights[i].init(shader, i);
        }
        for ( unsigned int i = 0; i < this->k_maxDlight; ++i ) {
            this->u_dlights[i].init(shader, i);
        }
        for ( unsigned int i = 0; i < this->k_maxPlight; ++i ) {
            this->u_plights[i].init(shader, i);
        }
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

    void UniInterfLightedMesh::slightCount(const unsigned int x) const {
        glUniform1i(this->u_slightCount, x);
    }

    void UniInterfLightedMesh::roughness(const float v) const {
        glUniform1f(this->u_roughness, v);
    }

    void UniInterfLightedMesh::metallic(const float v) const {
        glUniform1f(this->u_metallic, v);
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

}


// UniInterfAnime
namespace dal {

    UniInterfAnime::UniInterfAnime(const GLuint shader) {
#if ASSERT_UNILOC
        dalAssertm(3 == glGetAttribLocation(shader, "i_jointIDs"), "Uniloc i_jointIDs not found");
        dalAssertm(4 == glGetAttribLocation(shader, "i_weights"), "Uniloc i_weights not found");
#endif

        this->u_jointTransforms = getUniloc(shader, "u_jointTransforms[0]");

        for ( unsigned int i = 1; i < MAX_JOINTS_NUM; ++i ) {
            const auto id = "u_jointTransforms[{}]"_format(i);
            const auto loc = getUniloc(shader, id.c_str());
            if ( loc != this->u_jointTransforms + i ) {
                dalAbort("u_jointTransforms[] is not a continuous array.");
            }
        }
    }

    void UniInterfAnime::jointTransforms(const unsigned int index, const glm::mat4& mat) const {
        dalAssert(index < MAX_JOINTS_NUM);
        sendMatrix(this->u_jointTransforms + index, mat);
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
        , m_lightmaps(shader)
    {
        this->i_envmap.set(shader);
    }

}


// UnilocOverlay
namespace dal {

    UnilocOverlay::UnilocOverlay(const GLuint shader) {
        this->u_bottLeft = getUniloc(shader, "u_bottLeft");
        this->u_rectSize = getUniloc(shader, "u_rectSize");

        this->u_upsideDown_maskMap = getUniloc(shader, "u_upsideDown_maskMap");
        this->u_upsideDown_diffuseMap = getUniloc(shader, "m_upsideDown_diffuseMap");

        this->u_texOffset = getUniloc(shader, "u_texOffset");
        this->u_texScale = getUniloc(shader, "u_texScale");

        // Fragment shader

        this->u_color = getUniloc(shader, "u_color");

        this->m_diffuseMap.init(getUniloc(shader, "u_diffuseMap"), getUniloc(shader, "u_hasDiffuseMap"), g_texUnitReg["u_diffuseMap"]);
        this->m_maskMap.init(getUniloc(shader, "u_maskMap"), getUniloc(shader, "u_hasMaskMap"), g_texUnitReg["u_maskMap"]);
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

    void UnilocOverlay::bottomLeft(const glm::vec2& v) const {
        glUniform2f(this->u_bottLeft, v.x, v.y);
    }

    void UnilocOverlay::rectSize(const glm::vec2& v) const {
        glUniform2f(this->u_rectSize, v.x, v.y);
    }

    void UnilocOverlay::upsideDownDiffuseMap(const bool x) const {
        sendBool(this->u_upsideDown_diffuseMap, x);
    }

    void UnilocOverlay::upsideDownMaskMap(const bool x) const {
        sendBool(this->u_upsideDown_maskMap, x);
    }

    void UnilocOverlay::color(const glm::vec4& v) const {
        glUniform4f(this->u_color, v.x, v.y, v.z, v.w);
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
#if ASSERT_UNILOC
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
        this->u_fogColor = getUniloc(shader, "u_fogColor");
        this->u_skyboxTex.init(getUniloc(shader, "u_skyboxTex"), -2, g_texUnitReg["u_skyboxTex"]);
    }

    void UnilocSkybox::fogColor(const float x, const float y, const float z) const {
        glUniform3f(this->u_fogColor, x, y, z);
    }
    void UnilocSkybox::fogColor(const glm::vec3& v) const {
        this->fogColor(v.x, v.y, v.z);
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
