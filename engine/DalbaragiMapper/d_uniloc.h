#pragma once

#include "d_opengl_shader.h"


namespace dal::gl {

    class SamplerInterf {

    private:
        gl::int_t m_samplerLoc = -1;
        univar::Bool m_flagHas;
        int m_unitIndex = -1;

    public:
        SamplerInterf(void) = default;
        SamplerInterf(const gl::int_t samplerLoc, const gl::int_t flagHasLoc, const unsigned int unitIndex, const bool flagAssert = false);
        void init(const gl::int_t samplerLoc, const gl::int_t flagHasLoc, const unsigned int unitIndex, const bool flagAssert = false);

        gl::int_t getSamplerLoc(void) const;
        void sendFlagHas(const bool x) const;
        int getUnitIndex(void) const;

    };

}


namespace dal::gl {

    constexpr unsigned MAX_NUM_DLIGHT = 3;
    constexpr unsigned MAX_NUM_PLIGHT = 3;
    constexpr unsigned MAX_NUM_SLIGHT = 3;


    class UniInterf_Lighting {

    public:
        univar::Vec3 u_baseAmbient;

        univar::Float u_roughness;
        univar::Float u_metallic;

        univar::Int u_dlightCount;
        univar::Int u_plightCount;
        univar::Int u_slightCount;

        univar::Vec3 u_dlight_direcs[MAX_NUM_DLIGHT];
        univar::Vec3 u_dlight_colors[MAX_NUM_DLIGHT];
        univar::Mat4 u_dlight_projViewMat[MAX_NUM_DLIGHT];
        SamplerInterf u_dlight_shadowmap[MAX_NUM_DLIGHT];

        univar::Vec3 u_plight_poses[MAX_NUM_PLIGHT];
        univar::Vec3 u_plight_colors[MAX_NUM_PLIGHT];

        univar::Vec3 u_slight_poses[MAX_NUM_SLIGHT];
        univar::Vec3 u_slight_direcs[MAX_NUM_SLIGHT];
        univar::Vec3 u_slight_colors[MAX_NUM_SLIGHT];
        univar::Float u_slight_fadeStart[MAX_NUM_SLIGHT];
        univar::Float u_slight_fadeEnd[MAX_NUM_SLIGHT];
        univar::Mat4 u_slight_projViewMat[MAX_NUM_SLIGHT];
        SamplerInterf u_slight_shadowmap[MAX_NUM_SLIGHT];

    public:
        void init(const Shader& shader);

    };

    class UniInterf_Lightmap {

    public:
        SamplerInterf u_diffuseMap, u_roughnessMap, u_metallicMap;

    public:
        void init(const Shader& shader);

    };

}


namespace dal::gl {

    class UniRender_Static {

    public:
        UniInterf_Lighting i_lighting;
        UniInterf_Lightmap i_lightmap;

        univar::Mat4 u_projMat;
        univar::Mat4 u_viewMat;
        univar::Mat4 u_modelMat;

        univar::Vec3 u_viewPos;

    public:
        void init(const Shader& shader);

    };

}
