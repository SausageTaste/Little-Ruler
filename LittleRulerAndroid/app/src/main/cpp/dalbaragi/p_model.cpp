#include "p_model.h"


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

    void ModelStatic::renderGeneral(const UnilocGeneral& uniloc, const std::list<ActorInfo>& actors) const {
        if ( !this->isReady() ) return;

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(uniloc);
            if ( !unit.m_mesh.isReady() ) continue;

            for ( auto& inst : actors ) {
                auto mat = inst.getViewMat();
                glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
                unit.m_mesh.draw();
            }
        }
    }

    void ModelStatic::renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const {
        if ( !this->isReady() ) return;

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) continue;

            for ( auto& inst : actors ) {
                auto mat = inst.getViewMat();
                glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
                unit.m_mesh.draw();
            }
        }
    }

    void ModelStatic::destroyModel(void) {
        for ( auto& unit : this->m_renderUnits ) {
            unit.m_mesh.destroyData();
        }
    }

}


// ModelStatic
namespace dal {

    ModelAnimated::RenderUnit* ModelAnimated::addRenderUnit(void) {
        this->m_renderUnits.emplace_back();
        return &this->m_renderUnits.back();
    }

    bool ModelAnimated::isReady(void) const {
        for ( const auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) return false;
        }

        return true;
    }

    void ModelAnimated::renderGeneral(const UnilocGeneral& uniloc, const std::list<ActorInfo>& actors) const {
        if ( !this->isReady() ) return;

        for ( auto& unit : this->m_renderUnits ) {
            unit.m_material.sendUniform(uniloc);
            if ( !unit.m_mesh.isReady() ) continue;

            for ( auto& inst : actors ) {
                auto mat = inst.getViewMat();
                glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
                unit.m_mesh.draw();
            }
        }
    }

    void ModelAnimated::renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const {
        if ( !this->isReady() ) return;

        for ( auto& unit : this->m_renderUnits ) {
            if ( !unit.m_mesh.isReady() ) continue;

            for ( auto& inst : actors ) {
                auto mat = inst.getViewMat();
                glUniformMatrix4fv(uniloc.uModelMat, 1, GL_FALSE, &mat[0][0]);
                unit.m_mesh.draw();
            }
        }
    }

    void ModelAnimated::destroyModel(void) {
        for ( auto& unit : this->m_renderUnits ) {
            unit.m_mesh.destroyData();
        }
    }

}