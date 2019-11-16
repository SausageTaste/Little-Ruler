#pragma once

#include "u_loadinfo.h"
#include "u_fileclass.h"


namespace dal {

    struct ModelLoadInfo {
        binfo::Model m_model;
        std::unique_ptr<ICollider> m_detailedCol;
        std::vector<Animation> m_animations;
    };

    bool loadDalModel(const ResourceID& resID, ModelLoadInfo& info);

}
