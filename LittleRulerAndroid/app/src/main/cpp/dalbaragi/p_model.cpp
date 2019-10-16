#include "p_resource.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include "s_logger_god.h"
#include "u_pool.h"


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


    bool ModelStatic::isReady(void) const {
        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                return false;
            }
        }

        return true;
    }

    void ModelStatic::render(const UniInterfLightedMesh& unilocLighted, const UniInterfLightmaps& unilocLightmaps, const glm::mat4& modelMat) const {
        if ( !this->isReady() ) {
            return;
        }

        unilocLighted.modelMat(modelMat);

        for ( const auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(unilocLighted, unilocLightmaps);
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
            if ( !unit.m_mesh.isReady() ) {
                return false;
            }
        }

        return true;
    }


    void ModelAnimated::render(const UniInterfLightedMesh& unilocLighted, const UniInterfLightmaps& unilocLightmaps,
        const UniInterfAnime& unilocAnime, const glm::mat4 modelMat, const JointTransformArray& transformArr) const
    {
        if ( !this->isReady() ) return;

        transformArr.sendUniform(unilocAnime);

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(unilocLighted, unilocLightmaps);
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

}
