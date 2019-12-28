#include "d_scene.h"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>


namespace {

    // I think it doesn't work as i expected.
    glm::mat4 findRotation(const glm::vec3& from, const glm::vec3& to) {
        const auto nfrom = glm::normalize(from);
        const auto nto = glm::normalize(to);

        const auto middleAngle = acos(glm::dot(nfrom, nto));
        const auto rotAxis = glm::normalize(glm::cross(nfrom, nto));

        const glm::mat4 identity{ 1.f };

        return glm::rotate(identity, middleAngle, rotAxis);
    }

    glm::vec2 convertVecToEuler2(const glm::vec3& v) {
        const glm::vec3 up{ 0.0f, 1.0f, 0.0f };
        const auto v_n = glm::normalize(v);

        float y = glm::radians(90.0f) - acos(glm::dot(up, v_n));

        float x = -atan2(-v_n.z, v_n.x) + glm::radians(450.f);
        x = fmod(x, glm::radians(360.f));

        return glm::vec2{ x, y };
    }

    glm::vec2 rotateVec2(const glm::vec2& v, const float radians) {
        const auto sinVal = sin(radians);
        const auto cosVal = cos(radians);

        return glm::vec2{
            v.x * cosVal - v.y * sinVal,
            v.x * sinVal + v.y * cosVal
        };
    }

}



// Directional light
namespace dal {

    void DirectionalLight::setDirection(const float x, const float y, const float z) {
        this->setDirection(glm::vec3{ x, y, z });
    }

    void DirectionalLight::setDirection(const glm::vec3& direc) {
        this->m_direc = glm::normalize(direc);
    }

}


// Camera
namespace dal {

    glm::mat4 Scene::Camera::makeViewMat(void) const {
        const glm::vec3 pos = static_cast<glm::vec3>(this->m_pos);
        return glm::lookAt(pos, pos + this->m_viewDir, this->m_upDir);
    }

    void Scene::Camera::rotateVertical(const float angle) {
        const glm::mat4 identity{ 1.f };
        const auto right = glm::normalize(glm::cross(this->m_viewDir, this->m_upDir));
        const auto rot = glm::rotate(identity, -angle, right);
        this->m_viewDir = rot * glm::vec4{ this->m_viewDir, 1.f };
        this->m_viewDir = glm::normalize(this->m_viewDir);
    }

    void Scene::Camera::rotateHorizontal(const float angle) {
        const glm::mat4 identity{ 1.f };
        const auto rot = glm::rotate(identity, -angle, this->m_upDir);
        this->m_viewDir = rot * glm::vec4{ this->m_viewDir, 1.f };
        this->m_viewDir = glm::normalize(this->m_viewDir);
    }

    void Scene::Camera::moveHorizontalAlongView(const glm::vec3& direction) {
        const auto euler2 = convertVecToEuler2(this->m_viewDir);
        const auto rotated = rotateVec2(glm::vec2{ direction.x, direction.z }, euler2.x);
        this->m_pos.x += rotated.x;
        this->m_pos.z += rotated.y;
        this->m_pos.y += direction.y;
    }

}


// Scene
namespace dal {

    Scene::Scene(void) {
        this->m_activeCamera = &this->m_cameras.emplace_back();
    }

    void Scene::render(const gl::UniRender_Static& uniloc) const {
        uniloc.i_lighting.u_baseAmbient.send(0.2f, 0.2f, 0.2f);
        uniloc.i_lighting.u_roughness << 0.1f;
        uniloc.i_lighting.u_metallic << 0.0f;

        uniloc.i_lighting.u_dlightCount << 0;
        uniloc.i_lighting.u_slightCount << 0;

        uniloc.i_lighting.u_plightCount << 1;
        uniloc.i_lighting.u_plight_colors[0].send(3, 3, 3);
        uniloc.i_lighting.u_plight_poses[0].send(0, 0, 1);

        this->m_albedo.sendUniform(uniloc.i_lightmap.u_diffuseMap);
        this->m_roughness.sendUniform(uniloc.i_lightmap.u_roughnessMap);
        this->m_metallic.sendUniform(uniloc.i_lightmap.u_metallicMap);

        uniloc.u_viewMat << this->activeCam().makeViewMat();
        uniloc.u_modelMat << glm::mat4{ 1.f };
        uniloc.u_viewPos << this->activeCam().m_pos;

        for ( auto& [name, mesh] : this->m_meshes ) {
            mesh.m_glmesh.draw();
        }
    }

}
