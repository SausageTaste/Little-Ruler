#pragma once

#include "d_opengl_shader.h"


namespace dal::gl {

    constexpr unsigned MAX_NUM_DLIGHT = 3;
    constexpr unsigned MAX_NUM_SLIGHT = 3;
    constexpr unsigned MAX_NUM_PLIGHT = 3;


    class UniInterf_Lights {

    public:
        gl::univar::Vec3 u_baseAmbient;

        gl::univar::Int u_dlightCount;
        gl::univar::Int u_plightCount;
        gl::univar::Int u_slightCount;

        gl::univar::Vec3 u_dlight_direcs[MAX_NUM_DLIGHT];
        gl::univar::Vec3 u_dlight_colors[MAX_NUM_DLIGHT];

        gl::univar::Vec3 u_plight_poses[MAX_NUM_PLIGHT];
        gl::univar::Vec3 u_plight_colors[MAX_NUM_PLIGHT];

        gl::univar::Vec3 u_slight_poses[MAX_NUM_SLIGHT];
        gl::univar::Vec3 u_slight_direcs[MAX_NUM_SLIGHT];
        gl::univar::Vec3 u_slight_colors[MAX_NUM_SLIGHT];
        gl::univar::Float u_slight_fadeStart[MAX_NUM_SLIGHT];
        gl::univar::Float u_slight_fadeEnd[MAX_NUM_SLIGHT];

    public:
        void init(const Shader& shader);

    };


    class Uniloc_Static {

    public:
        gl::univar::Mat4 u_projMat;

    public:
        void init(const Shader& shader);

    };

}
