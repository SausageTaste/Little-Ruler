#include "d_opengl_state.h"

#include <string>
#include <iostream>

#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>

#include <d_shaderProcessor.h>
#include <d_filesystem.h>


namespace {

    auto glfunc(void) {
        return QOpenGLContext::currentContext()->extraFunctions();
    }

}


// GLSL
namespace {

    const std::string glsl_header = R"(
#version 300 es
precision highp float;
)";

    constexpr char glsl_lights_i[] = R"(

uniform       vec3  u_baseAmbient;

uniform highp int   u_dlightCount;
uniform       int   u_plightCount;
uniform highp int   u_slightCount;

uniform       vec3  u_dlight_direcs[3];
uniform       vec3  u_dlight_colors[3];

uniform       vec3  u_plight_poses[3];
uniform       vec3  u_plight_colors[3];

uniform       vec3  u_slight_poses[3];
uniform       vec3  u_slight_direcs[3];
uniform       vec3  u_slight_colors[3];
uniform       float u_slight_fadeStart[3];
uniform       float u_slight_fadeEnd[3];


// General

float _calcDistAttenu(float fragDist, float constant, float linear, float quadratic) {
    return 1.0 / (constant + linear * fragDist + quadratic * (fragDist * fragDist));
}


// Directional

vec3 calcRadiance_dlight(int i) {
    return u_dlight_colors[i];
}

vec3 calcToLight_dlight(int i) {
    return -u_dlight_direcs[i];
}


// Point

vec3 calcRadiance_plight(int i, vec3 fragPos) {
    float attenFactor = _calcDistAttenu(distance(fragPos, u_plight_poses[i]), 1.0, 0.09, 0.032);
    return u_plight_colors[i] * attenFactor;
}

vec3 calcToLight_plight(int i, vec3 fragPos) {
    return normalize(u_plight_poses[i] - fragPos);
}


// Spot

float _calcSlightAtten(int i, vec3 fragPos) {
    vec3 fragToLight_n = normalize(u_slight_poses[i] - fragPos);
    float theta        = dot(-fragToLight_n, u_slight_direcs[i]);
    float epsilon      = u_slight_fadeStart[i] - u_slight_fadeEnd[i];

    return clamp((theta - u_slight_fadeEnd[i]) / epsilon, 0.0, 1.0);
}

vec3 calcRadiance_slight(int i, vec3 fragPos) {
    return u_slight_colors[i] * _calcSlightAtten(i, fragPos);
}

vec3 calcToLight_slight(int i, vec3 fragPos) {
    return normalize(u_slight_poses[i] - fragPos);
}

)";

    constexpr char glsl_static_v[] = R"(

layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_texCoord;
layout (location = 2) in vec3 i_normal;


uniform mat4 u_projMat;


void main(void) {
    gl_Position = u_projMat * vec4(i_position, 1.0);
}

)";
    
    constexpr char glsl_static_f[] = R"(

out vec4 f_color;


void main(void) {
    f_color = vec4(0.0, 1.0, 0.0, 1.0);
}

)";

}


namespace dal::gl {

    void State::init(void) {
        dal::ShaderPreprocessor loader;

        {
            this->m_static.init(loader["r_static.vert"], loader["r_static.frag"]);
            this->u_static.init(this->m_static);
        }
    }


    void State::setViewportSize(const gl::int_t width, const gl::int_t height) {
        glfunc()->glViewport(0, 0, width, height);
        this->m_viewportSize.first = width;
        this->m_viewportSize.second = height;
    }


    const UniRender_Static& State::use_static(void) const {
        this->m_static.use();
        return this->u_static;
    }

}
