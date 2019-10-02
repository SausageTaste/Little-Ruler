#include "p_resource.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"
#include "u_objparser.h"
#include "u_pool.h"
#include "s_configs.h"


#define BLOCKY_TEXTURE 0


using namespace fmt::literals;


// Pools
namespace {

    dal::BlockAllocator<dal::ModelStatic, 20> g_pool_modelStatic;
    dal::BlockAllocator<dal::ModelAnimated, 20> g_pool_modelAnim;

}


// ModelStatic
namespace dal {

    void* ModelStatic::operator new(size_t size) {
        auto allocated = g_pool_modelStatic.alloc();
        if ( nullptr == allocated ) {
            throw std::bad_alloc{};
        }
        else {
            return allocated;
        }
    }

    void ModelStatic::operator delete(void* ptr) {
        g_pool_modelStatic.free(reinterpret_cast<ModelStatic*>(ptr));
    }

    /*
    void ModelStatic::init(const ResourceID& resID, const binfo::ModelStatic& info, ResourceMaster& resMas) {
        this->invalidate();

        this->setModelResID(std::move(resID));
        this->setBoundingBox(info.m_aabb);

        for ( auto& unitInfo : info.m_renderUnits ) {
            auto& unit = this->m_renderUnits.emplace_back();

            unit.m_mesh.buildData(
                unitInfo.m_mesh.m_vertices.data(), unitInfo.m_mesh.m_vertices.size(),
                unitInfo.m_mesh.m_texcoords.data(), unitInfo.m_mesh.m_texcoords.size(),
                unitInfo.m_mesh.m_normals.data(), unitInfo.m_mesh.m_normals.size()
            );
            unit.m_meshName = unitInfo.m_name;

            unit.m_material.m_diffuseColor = unitInfo.m_material.m_diffuseColor;
            unit.m_material.m_shininess = unitInfo.m_material.m_shininess;
            unit.m_material.m_specularStrength = unitInfo.m_material.m_specStrength;

            if ( !unitInfo.m_material.m_diffuseMap.empty() ) {
                ResourceID texResID{ unitInfo.m_material.m_diffuseMap };
                texResID.setPackageIfEmpty(resID.getPackage());
                auto tex = resMas.orderTexture(texResID);
                unit.m_material.setDiffuseMap(tex);
            }
        }

        this->m_bounding.reset(new ColAABB{ info.m_aabb });
    }

    void ModelStatic::init(const binfo::ModelDefined& info, ResourceMaster& resMas) {
        this->invalidate();

        this->setBoundingBox(info.m_boundingBox);

        auto& unitInfo = info.m_renderUnit;
        auto& unit = this->m_renderUnits.emplace_back();

        unit.m_mesh.buildData(
            unitInfo.m_mesh.m_vertices.data(), unitInfo.m_mesh.m_vertices.size(),
            unitInfo.m_mesh.m_texcoords.data(), unitInfo.m_mesh.m_texcoords.size(),
            unitInfo.m_mesh.m_normals.data(), unitInfo.m_mesh.m_normals.size()
        );
        unit.m_meshName = unitInfo.m_name;

        unit.m_material.m_diffuseColor = unitInfo.m_material.m_diffuseColor;
        unit.m_material.m_shininess = unitInfo.m_material.m_shininess;
        unit.m_material.m_specularStrength = unitInfo.m_material.m_specStrength;
        unit.m_material.setTexScale(info.m_renderUnit.m_material.m_texSize.x, info.m_renderUnit.m_material.m_texSize.y);

        if ( !unitInfo.m_material.m_diffuseMap.empty() ) {
            auto tex = resMas.orderTexture(unitInfo.m_material.m_diffuseMap);
            unit.m_material.setDiffuseMap(tex);
        }
    }
    */

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

        unilocLighted.modelMat(modelMat);

        for ( const auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(unilocLighted, samplerInterf);
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }
            unit.m_mesh.draw();
        }
    }

    void ModelStatic::renderDepth(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const {
        if ( !this->isReady() ) {
            return;
        }

        unilocGeometry.modelMat(modelMat);

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }
            unit.m_mesh.draw();
        }
    }

}


// ModelAnimated
namespace dal {

    void* ModelAnimated::operator new(size_t size) {
        auto allocated = g_pool_modelAnim.alloc();
        if ( nullptr == allocated ) {
            throw std::bad_alloc{};
        }
        else {
            return allocated;
        }
    }

    void ModelAnimated::operator delete(void* ptr) {
        g_pool_modelAnim.free(reinterpret_cast<ModelAnimated*>(ptr));
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
        const UniInterfAnime& unilocAnime, const glm::mat4 modelMat, const JointTransformArray& transformArr)
    {
        if ( !this->isReady() ) return;

        transformArr.sendUniform(unilocAnime);

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(unilocLighted, samplerInterf);
            if ( !unit.m_mesh.isReady() ) continue;

            unilocLighted.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::renderDepth(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
        const JointTransformArray& transformArr) const {
        if ( !this->isReady() ) return;

        transformArr.sendUniform(unilocAnime);

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) continue;

            unilocGeometry.modelMat(modelMat);
            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::invalidate(void) {
        this->m_renderUnits.clear();
    }

}


/*
namespace dal {

    void ModelStaticHandle::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const {
        this->getPimpl()->m_model->render(unilocLighted, samplerInterf, modelMat);
    }

    void ModelStaticHandle::renderDepth(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const {
        this->getPimpl()->m_model->renderDepth(unilocGeometry, modelMat);
    }

}


namespace dal {

    const SkeletonInterface& ModelAnimatedHandle::getSkeletonInterf(void) const {
        return this->getPimpl()->m_model->getSkeletonInterf();
    }

    const std::vector<Animation>& ModelAnimatedHandle::getAnimations(void) const {
        return this->getPimpl()->m_model->getAnimations();
    }

    const glm::mat4& ModelAnimatedHandle::getGlobalInvMat(void) const {
        return this->getPimpl()->m_model->getGlobalInvMat();
    }

    void ModelAnimatedHandle::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const UniInterfAnime& unilocAnime,
        const glm::mat4 modelMat, const JointTransformArray& transformArr)
    {
        this->getPimpl()->m_model->render(unilocLighted, samplerInterf, unilocAnime, modelMat, transformArr);
    }

    void ModelAnimatedHandle::renderDepth(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
        const JointTransformArray& transformArr) const
    {
        this->getPimpl()->m_model->renderDepth(unilocGeometry, unilocAnime, modelMat, transformArr);
    }

}
*/
