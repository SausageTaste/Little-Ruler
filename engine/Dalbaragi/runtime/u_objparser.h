#pragma once

#include "u_loadinfo.h"


namespace dal {

    struct ModelLoadInfo {
        binfo::Model m_model;
        std::unique_ptr<ICollider> m_detailedCol;
        std::vector<Animation> m_animations;
    };

    bool loadDalModel(const char* const respath, ModelLoadInfo& info);

}
