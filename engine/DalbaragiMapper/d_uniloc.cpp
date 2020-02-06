#include "d_uniloc.h"

#include <unordered_map>


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
namespace dal::gl {

    SamplerInterf::SamplerInterf(const gl::int_t samplerLoc, const gl::int_t flagHasLoc, const unsigned int unitIndex, const bool flagAssert) {
        this->init(samplerLoc, flagHasLoc, unitIndex, flagAssert);
    }

    void SamplerInterf::init(const gl::int_t samplerLoc, const gl::int_t flagHasLoc, const unsigned int unitIndex, const bool flagAssert) {
        this->m_samplerLoc = samplerLoc;
        this->m_flagHas >> flagHasLoc;
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
            this->m_flagHas.invalidate();
        }
    }

    gl::int_t SamplerInterf::getSamplerLoc(void) const {
        return this->m_samplerLoc;
    }

    void SamplerInterf::sendFlagHas(const bool x) const {
        if ( this->m_flagHas ) {
            this->m_flagHas << x;
        }
    }

    int SamplerInterf::getUnitIndex(void) const {
        return this->m_unitIndex;
    }

}


namespace dal::gl {

    void UniInterf_Lighting::init(const Shader& shader) {
        using namespace std;

        u_baseAmbient >> shader.getUniloc("u_baseAmbient");

        u_roughness >> shader.getUniloc("u_roughness");
        u_metallic >> shader.getUniloc("u_metallic");

        u_dlightCount >> shader.getUniloc("u_dlightCount");
        u_plightCount >> shader.getUniloc("u_plightCount");
        u_slightCount >> shader.getUniloc("u_slightCount");

        for ( unsigned i = 0; i < MAX_NUM_DLIGHT; ++i ) {
            const auto index = "["s + to_string(i) + ']';
            u_dlight_direcs[i] >> shader.getUniloc("u_dlight_direcs"s + index);
            u_dlight_colors[i] >> shader.getUniloc("u_dlight_colors"s + index);
            u_dlight_projViewMat[i] >> shader.getUniloc("u_dlight_projViewMat"s + index);

            const auto mapid = "u_dlight_shadowmap"s + index;
            u_dlight_shadowmap[i].init(shader.getUniloc(mapid), -2, g_texUnitReg[mapid]);
        }

        for ( unsigned i = 0; i < MAX_NUM_PLIGHT; ++i ) {
            const auto index = "["s + to_string(i) + ']';
            u_plight_poses[i] >> shader.getUniloc("u_plight_poses"s + index);
            u_plight_colors[i] >> shader.getUniloc("u_plight_colors"s + index);
        }

        for ( unsigned i = 0; i < MAX_NUM_SLIGHT; ++i ) {
            const auto index = "["s + to_string(i) + ']';
            u_slight_poses[i] >> shader.getUniloc("u_slight_poses"s + index);
            u_slight_direcs[i] >> shader.getUniloc("u_slight_direcs"s + index);
            u_slight_colors[i] >> shader.getUniloc("u_slight_colors"s + index);
            u_slight_fadeStart[i] >> shader.getUniloc("u_slight_fadeStart"s + index);
            u_slight_fadeEnd[i] >> shader.getUniloc("u_slight_fadeEnd"s + index);
            u_slight_projViewMat[i] >> shader.getUniloc("u_slight_projViewMat"s + index);

            const auto mapid = "u_slight_shadowmap"s + index;
            u_slight_shadowmap[i].init(shader.getUniloc(mapid), -2, g_texUnitReg[mapid]);
        }
    }

    void UniInterf_Lightmap::init(const Shader& shader) {
        this->u_diffuseMap.init(
            shader.getUniloc("u_diffuseMap"),
            -2,
            g_texUnitReg["u_diffuseMap"]
        );

        this->u_roughnessMap.init(
            shader.getUniloc("u_roughnessMap"),
            shader.getUniloc("u_hasRoughnessMap"),
            g_texUnitReg["u_roughnessMap"]
        );

        this->u_metallicMap.init(
            shader.getUniloc("u_metallicMap"),
            shader.getUniloc("u_hasMetallicMap"),
            g_texUnitReg["u_metallicMap"]
        );
    }

    void UniRender_Static::init(const Shader& shader) {
        this->i_lighting.init(shader);
        this->i_lightmap.init(shader);

        this->u_projMat >> shader.getUniloc("u_projMat");
        this->u_viewMat >> shader.getUniloc("u_viewMat");
        this->u_modelMat >> shader.getUniloc("u_modelMat");

        this->u_viewPos >> shader.getUniloc("u_viewPos");
    }

}
