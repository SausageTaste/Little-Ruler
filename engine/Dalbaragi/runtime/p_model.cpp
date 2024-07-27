#include "p_resource.h"

#include <spdlog/fmt/fmt.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>
#include <d_pool.h>


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


    void ModelStatic::render(const UniRender_Static& uniloc) const {
        if ( !this->isReady() ) {
            return;
        }

        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }

            unit.m_material.sendUniform(uniloc.i_lighting);
            unit.m_material.sendUniform(uniloc.i_lightmap);

            unit.m_mesh.draw();
        }
    }

    void ModelStatic::render(const UniRender_StaticOnWater& uniloc) const {
        if ( !this->isReady() ) {
            return;
        }

        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }

            unit.m_material.sendUniform(uniloc.i_lighting);
            unit.m_material.sendUniform(uniloc.i_lightmap);

            unit.m_mesh.draw();
        }
    }

    void ModelStatic::render(void) const {
        if ( !this->isReady() ) {
            return;
        }

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

    bool ModelAnimated::isReady(void) const {
        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                return false;
            }
        }

        return true;
    }


    void ModelAnimated::render(const UniRender_Animated uniloc, const JointTransformArray& transformArr) const {
        if ( !this->isReady() ) {
            return;
        }

        transformArr.sendUniform(uniloc.i_skeleton);

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }

            unit.m_material.sendUniform(uniloc.i_lighting);
            unit.m_material.sendUniform(uniloc.i_lightmap);
            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::render(const UniRender_AnimatedDepth& uniloc, const JointTransformArray& transformArr) const {
        if ( !this->isReady() ) return;

        transformArr.sendUniform(uniloc.i_skeleton);

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }

            unit.m_mesh.draw();
        }
    }

    void ModelAnimated::render(const UniRender_AnimatedOnWater& uniloc, const JointTransformArray& transformArr) const {
        if ( !this->isReady() ) {
            return;
        }

        transformArr.sendUniform(uniloc.i_skeleton);

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) {
                continue;
            }

            unit.m_material.sendUniform(uniloc.i_lighting);
            unit.m_material.sendUniform(uniloc.i_lightmap);
            unit.m_mesh.draw();
        }
    }

}
