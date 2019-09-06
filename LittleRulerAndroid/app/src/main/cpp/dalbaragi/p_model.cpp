#include "p_resource.h"

#include <limits>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/matrix4x4.h>

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
    void ModelStatic::init(const ResourceID& resID, const loadedinfo::ModelStatic& info, ResourceMaster& resMas) {
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

    void ModelStatic::init(const loadedinfo::ModelDefined& info, ResourceMaster& resMas) {
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

    void ModelStatic::invalidate(void) {
        this->m_renderUnits.clear();
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

    void ModelAnimated::renderDepthMap(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
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

    /*
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
    */

}


/*
// ModelStaticHandle
namespace dal {

    struct ModelStaticHandleImpl {
        std::unique_ptr<ModelStatic> m_model;
        unsigned int m_refCount = 1;
    };

    dal::StaticPool<dal::ModelStaticHandleImpl, 20> g_staticModelCtrlBlckPool;


    ModelStaticHandle::ModelStaticHandle(void)
        : m_pimpl(g_staticModelCtrlBlckPool.alloc())
    {

    }

    ModelStaticHandle::ModelStaticHandle(dal::ModelStatic* const model)
        : m_pimpl(g_staticModelCtrlBlckPool.alloc())
    {
        this->m_pimpl->m_model.reset(model);
    }

    ModelStaticHandle::~ModelStaticHandle(void) {
        if ( nullptr == this->m_pimpl ) {
            return;
        }

        --this->m_pimpl->m_refCount;
        if ( 0 == this->m_pimpl->m_refCount ) {
            g_staticModelCtrlBlckPool.free(this->m_pimpl);
            this->m_pimpl = nullptr;
        }
    }

    ModelStaticHandle::ModelStaticHandle(ModelStaticHandle&& other) noexcept
        : m_pimpl(nullptr)
    {
        std::swap(this->m_pimpl, other.m_pimpl);
    }

    ModelStaticHandle::ModelStaticHandle(const ModelStaticHandle& other)
        : m_pimpl(other.m_pimpl)
    {
        ++this->m_pimpl->m_refCount;
    }

    ModelStaticHandle& ModelStaticHandle::operator=(const ModelStaticHandle& other) {
        this->m_pimpl = other.m_pimpl;
        ++this->m_pimpl->m_refCount;
        return *this;
    }

    ModelStaticHandle& ModelStaticHandle::operator=(ModelStaticHandle&& other) noexcept {
        std::swap(this->m_pimpl, other.m_pimpl);
        return *this;
    }


    bool ModelStaticHandle::operator==(ModelStaticHandle& other) const {
        dalAssert(nullptr != this->m_pimpl);
        dalAssert(nullptr != other.m_pimpl);

        if ( nullptr == this->m_pimpl->m_model ) {
            return false;
        }
        else if ( nullptr == other.m_pimpl->m_model ) {
            return false;
        }
        else if ( this->m_pimpl->m_model != other.m_pimpl->m_model ) {
            return false;
        }
        else {
            return true;
        }
    }

    void ModelStaticHandle::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const {
        dalAssert(nullptr != this->m_pimpl);
        if ( nullptr != this->m_pimpl->m_model ) {
            this->m_pimpl->m_model->render(unilocLighted, samplerInterf, modelMat);
        }
        else {
            dalAbort("Not implemented.");
        }
    }

    void ModelStaticHandle::renderDepthMap(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const {
        dalAssert(nullptr != this->m_pimpl);
        if ( nullptr != this->m_pimpl->m_model ) {
            this->m_pimpl->m_model->renderDepthMap(unilocGeometry, modelMat);
        }
        else {
            dalAbort("Not implemented.");
        }
    }

    unsigned int ModelStaticHandle::getRefCount(void) const {
        return this->m_pimpl->m_refCount;
    }

    const ResourceID& ModelStaticHandle::getResID(void) const {
        dalAssert(nullptr != this->m_pimpl);
        if ( nullptr != this->m_pimpl->m_model ) {
            return this->m_pimpl->m_model->m_resID;
        }
        else {
            dalAbort("Not implemented.");
        }
    }

    const ICollider* ModelStaticHandle::getBounding(void) const {
        dalAssert(nullptr != this->m_pimpl);
        return this->m_pimpl->m_model->m_bounding.get();
    }

    const ICollider* ModelStaticHandle::getDetailed(void) const {
        dalAssert(nullptr != this->m_pimpl);
        if ( this->m_pimpl->m_model->m_detailed ) {
            return this->m_pimpl->m_model->m_detailed.get();
        }
        else {
            return nullptr;
        }
    }

}
*/


namespace dal {

    ModelStaticHandle::ModelStaticHandle(ModelStatic* const model) {
        this->getPimpl()->m_model.reset(model);
    }

    void ModelStaticHandle::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const glm::mat4& modelMat) const {
        this->getPimpl()->m_model->render(unilocLighted, samplerInterf, modelMat);
    }

    void ModelStaticHandle::renderDepthMap(const UniInterfGeometry& unilocGeometry, const glm::mat4& modelMat) const {
        this->getPimpl()->m_model->renderDepthMap(unilocGeometry, modelMat);
    }

}


namespace dal {

    ModelAnimatedHandle::ModelAnimatedHandle(ModelAnimated* const model) {
        this->getPimpl()->m_model.reset(model);
    }

    void ModelAnimatedHandle::render(const UniInterfLightedMesh& unilocLighted, const SamplerInterf& samplerInterf, const UniInterfAnime& unilocAnime,
        const glm::mat4 modelMat, const JointTransformArray& transformArr)
    {
        this->getPimpl()->m_model->render(unilocLighted, samplerInterf, unilocAnime, modelMat, transformArr);
    }

    void ModelAnimatedHandle::renderDepthMap(const UniInterfGeometry& unilocGeometry, const UniInterfAnime& unilocAnime, const glm::mat4 modelMat,
        const JointTransformArray& transformArr) const
    {
        this->getPimpl()->m_model->renderDepthMap(unilocGeometry, unilocAnime, modelMat, transformArr);
    }

}
