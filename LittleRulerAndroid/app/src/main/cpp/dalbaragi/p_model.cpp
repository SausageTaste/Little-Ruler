#include "p_model.h"

#include <glm/gtc/matrix_transform.hpp>
#include <assimp/matrix4x4.h>

#include "s_logger_god.h"

namespace {

    inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from) {
        glm::mat4 to;
        using GLfloat = float;

        to[0][0] = (GLfloat)from->a1; to[0][1] = (GLfloat)from->b1;  to[0][2] = (GLfloat)from->c1; to[0][3] = (GLfloat)from->d1;
        to[1][0] = (GLfloat)from->a2; to[1][1] = (GLfloat)from->b2;  to[1][2] = (GLfloat)from->c2; to[1][3] = (GLfloat)from->d2;
        to[2][0] = (GLfloat)from->a3; to[2][1] = (GLfloat)from->b3;  to[2][2] = (GLfloat)from->c3; to[2][3] = (GLfloat)from->d3;
        to[3][0] = (GLfloat)from->a4; to[3][1] = (GLfloat)from->b4;  to[3][2] = (GLfloat)from->c4; to[3][3] = (GLfloat)from->d4;

        return to;
    }

}


// IModel
namespace dal {

    void IModel::setModelResID(const ResourceID& resID) {
        this->m_modelResID = resID;
    }

    const ResourceID& IModel::getModelResID(void) const {
        return this->m_modelResID;
    }

    void IModel::setBoundingBox(const AxisAlignedBoundingBox& box) {
        this->m_boundingBox = box;
    }

    const AxisAlignedBoundingBox& IModel::getBoundingBox(void) const {
        return this->m_boundingBox;
    }

}


// ModelStatic
namespace dal {

    ModelStatic::RenderUnit* ModelStatic::addRenderUnit(void) {
        this->m_renderUnits.emplace_back();
        return &this->m_renderUnits.back();
    }

    bool ModelStatic::isReady(void) const {
        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) return false;
        }

        return true;
    }

    void ModelStatic::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const {
        if ( !this->isReady() ) {
            return;
        }

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(unilocLighted, samplerInterf);
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }

            unilocLighted.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelStatic::renderDepthMap(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const {
        if ( !this->isReady() ) {
            return;
        }

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }

            unilocGeometry.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelStatic::destroyModel(void) {
        for ( auto& unit : this->m_renderUnits ) {
            unit.m_mesh.destroyData();
        }
    }

}


// ModelAnimated
namespace dal {

    ModelAnimated::RenderUnit* ModelAnimated::addRenderUnit(void) {
        this->m_renderUnits.emplace_back();
        return &this->m_renderUnits.back();
    }

    void ModelAnimated::setSkeletonInterface(SkeletonInterface&& joints) {
        this->m_jointInterface = std::move(joints);
    }

    void ModelAnimated::setAnimations(std::vector<Animation>&& animations) {
        this->m_animations = std::move(animations);
    }

    void ModelAnimated::setGlobalMat(const glm::mat4 mat) {
        this->m_globalInvMat = glm::inverse(mat);
    }

    bool ModelAnimated::isReady(void) const {
        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) return false;
        }

        return true;
    }

    void ModelAnimated::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf,
        const UniInterfAnime& unilocAnime, const glm::mat4 modelMat)
    {
        if ( !this->isReady() ) return;

        this->m_jointInterface.sendUniform(unilocAnime);

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(unilocLighted, samplerInterf);
            if ( !unit.m_mesh.isReady() ) continue;

            unilocLighted.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::renderDepthMap(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat) const {
        if ( !this->isReady() ) return;

        this->m_jointInterface.sendUniform(unilocAnime);

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) continue;

            unilocGeometry.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::destroyModel(void) {
        for ( auto& unit : this->m_renderUnits ) {
            unit.m_mesh.destroyData();
        }
    }

    void ModelAnimated::updateAnimation0(void) {
        if ( this->m_animations.empty() ) {
            return;
        }

        const auto& anim = this->m_animations.back();
        const auto elapsed = this->m_animLocalTimer.getElapsed();
        const auto animDuration = anim.getDurationInTick();
        const auto animTickPerSec = anim.getTickPerSec();

        float TimeInTicks = elapsed * animTickPerSec;
        const auto animTick = fmod(TimeInTicks, animDuration);

        anim.sample(animTick, this->m_jointInterface, this->m_globalInvMat);
    }

}