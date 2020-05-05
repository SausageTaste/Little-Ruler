#include "p_uniloc.h"

#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include <d_logger.h>

#include "p_dalopengl.h"


#define ASSERT_UNILOC true


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
        this->u_envmapPos = getUniloc(shader, "u_envmapPos");
        this->u_envmap.init(getUniloc(shader, "u_envmap"), getUniloc(shader, "u_hasEnvmap"), g_texUnitReg["u_envmap"]);

        this->u_numPlanes = getUniloc(shader, "u_numPlanes");
        this->u_planes = getUniloc(shader, "u_planes");
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
        this->u_plight_maxDists = getUniloc(shader, "u_plight_maxDist");

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
        this->u_slight_maxDists = getUniloc(shader, "u_slight_maxDists");
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
        this->u_normalMap.init(getUniloc(shader, "u_normalMap"), getUniloc(shader, "u_hasNormalMap"), g_texUnitReg["u_normalMap"]);
    }

    void UniInterf_Skeleton::set(const GLuint shader) {
        this->u_jointTrans = getUniloc(shader, "u_jointTrans[0]");

        for ( unsigned int i = 1; i < MAX_JOINTS_NUM; ++i ) {
            const auto id = "u_jointTrans[{}]"_format(i);
            const auto loc = getUniloc(shader, id.c_str());
            if ( loc != this->u_jointTrans + i ) {
                dalAbort("u_jointTrans[] is not a continuous array.");
            }
        }
    }


    void UniRender_Static::set(const GLuint shader) {
        this->i_lighting.set(shader);
        this->i_lightmap.set(shader);
        this->i_envmap.set(shader);

        this->u_projMat = getUniloc(shader, "u_projMat");
        this->u_viewMat = getUniloc(shader, "u_viewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");

        this->u_viewPos = getUniloc(shader, "u_viewPos");
    }

    void UniRender_Animated::set(const GLuint shader) {
        this->i_lighting.set(shader);
        this->i_lightmap.set(shader);
        this->i_envmap.set(shader);
        this->i_skeleton.set(shader);

        this->u_projMat = getUniloc(shader, "u_projMat");
        this->u_viewMat = getUniloc(shader, "u_viewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");

        this->u_viewPos = getUniloc(shader, "u_viewPos");
    }

    void UniRender_StaticDepth::set(const GLuint shader) {
        this->u_projMat = getUniloc(shader, "u_projMat");
        this->u_viewMat = getUniloc(shader, "u_viewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");
    }

    void UniRender_AnimatedDepth::set(const GLuint shader) {
        this->i_skeleton.set(shader);

        this->u_projMat = getUniloc(shader, "u_projMat");
        this->u_viewMat = getUniloc(shader, "u_viewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");
    }

    void UniRender_StaticOnWater::set(const GLuint shader) {
        this->i_lighting.set(shader);
        this->i_lightmap.set(shader);

        this->u_projMat = getUniloc(shader, "u_projMat");
        this->u_viewMat = getUniloc(shader, "u_viewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");

        this->u_viewPos = getUniloc(shader, "u_viewPos");

        this->u_clipPlane = getUniloc(shader, "u_clipPlane");
    }

    void UniRender_AnimatedOnWater::set(const GLuint shader) {
        this->i_lighting.set(shader);
        this->i_lightmap.set(shader);
        this->i_skeleton.set(shader);

        this->u_projMat = getUniloc(shader, "u_projMat");
        this->u_viewMat = getUniloc(shader, "u_viewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");

        this->u_viewPos = getUniloc(shader, "u_viewPos");

        this->u_clipPlane = getUniloc(shader, "u_clipPlane");
    }

    void UniRender_FillScreen::set(const GLuint shader) {
        this->u_texture.init(getUniloc(shader, "u_texture"), -2, g_texUnitReg["u_texture"]);
    }

    void UniRender_Water::set(const GLuint shader) {
        this->i_lighting.set(shader);

        this->u_viewPos = getUniloc(shader, "u_viewPos");

        this->u_projMat = getUniloc(shader, "u_projMat");
        this->u_viewMat = getUniloc(shader, "u_viewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");

        this->u_texScale = getUniloc(shader, "u_texScale");

        this->u_bansaImg.init(getUniloc(shader, "u_bansaImg"), -2, g_texUnitReg["u_bansaImg"]);
        this->u_gooljulImg.init(getUniloc(shader, "u_gooljulImg"), -2, g_texUnitReg["u_gooljulImg"]);
        this->u_dudvMap.init(getUniloc(shader, "u_dudvMap"), -2, g_texUnitReg["u_dudvMap"]);
        this->u_normalMap.init(getUniloc(shader, "u_normalMap"), -2, g_texUnitReg["u_normalMap"]);
        this->u_depthMap.init(getUniloc(shader, "u_depthMap"), -2, g_texUnitReg["u_depthMap"]);

        this->u_dudvMoveFactor = getUniloc(shader, "u_dudvMoveFactor");
        this->u_waveStrength = getUniloc(shader, "u_waveStrength");
        this->u_darkestDepthPoint = getUniloc(shader, "u_darkestDepthPoint");
        this->u_reflectance = getUniloc(shader, "u_reflectance");
        this->u_deepColor = getUniloc(shader, "u_deepColor");
    }

    void UniRender_Skybox::set(const GLuint shader) {
        this->i_lighting.set(shader);

        this->u_projViewMat = getUniloc(shader, "u_projViewMat");
        this->u_modelMat = getUniloc(shader, "u_modelMat");

        this->u_viewPos = getUniloc(shader, "u_viewPos");
        this->u_viewPosActual = getUniloc(shader, "u_viewPosActual");
        this->u_skyboxTex.init(getUniloc(shader, "u_skyboxTex"), -2, g_texUnitReg["u_skyboxTex"]);

        this->u_dlight_direc = getUniloc(shader, "u_dlight_direc");
    }

    void UniRender_Overlay::set(const GLuint shader) {
        this->u_bottomLeft = getUniloc(shader, "u_bottomLeft");
        this->u_rectSize = getUniloc(shader, "u_rectSize");

        this->u_texOffset = getUniloc(shader, "u_texOffset");
        this->u_texScale = getUniloc(shader, "u_texScale");

        this->u_yFlip_colorMap = getUniloc(shader, "u_yFlip_colorMap");
        this->u_yFlip_maskMap = getUniloc(shader, "u_yFlip_maskMap");

        this->u_colorDefault = getUniloc(shader, "u_colorDefault");

        this->u_colorMap.init(getUniloc(shader, "u_colorMap"), getUniloc(shader, "u_hasColorMap"), g_texUnitReg["u_colorMap"]);
        this->u_maskMap.init(getUniloc(shader, "u_maskMap"), getUniloc(shader, "u_hasMaskMap"), g_texUnitReg["u_maskMap"]);
    }

}
