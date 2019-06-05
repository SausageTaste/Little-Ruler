#pragma once

#include <string>
#include <vector>

#include "p_meshStatic.h"
#include "m_collider.h"
#include "u_fileclass.h"
#include "p_uniloc.h"


namespace dal {

    class IModel {

    private:
        ResourceID m_modelResID;
        AxisAlignedBoundingBox m_boundingBox;

    public:
        void setModelResID(const ResourceID& resID);
        const ResourceID& getModelResID(void) const;

        void setBoundingBox(const AxisAlignedBoundingBox& box);
        const AxisAlignedBoundingBox& getBoundingBox(void) const;

    };


    class Model : public IModel {

    private:
        struct RenderUnit {
            std::string m_meshName;
            dal::MeshStatic m_mesh;
            dal::Material m_material;
        };

        std::vector<RenderUnit> m_renderUnits;

    public:

        RenderUnit* addRenderUnit(void);

        bool isReady(void) const;

        void renderGeneral(const UnilocGeneral& uniloc, const std::list<ActorInfo>& actors) const;
        void renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const;

        void destroyModel(void);

    };


    class ModelAnimated : public IModel {

    private:
        struct RenderUnit {
            std::string m_meshName;
            dal::MeshAnimated m_mesh;
            dal::Material m_material;
        };

        std::vector<RenderUnit> m_renderUnits;

    public:
        RenderUnit* addRenderUnit(void);

        bool isReady(void) const;

        void renderGeneral(const UnilocGeneral& uniloc, const std::list<ActorInfo>& actors) const;
        void renderDepthMap(const UnilocDepthmp& uniloc, const std::list<ActorInfo>& actors) const;

        void destroyModel(void);

    };

}