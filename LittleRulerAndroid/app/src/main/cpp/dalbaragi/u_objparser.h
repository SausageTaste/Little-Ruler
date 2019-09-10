#pragma once

#include "u_loadinfo.h"
#include "u_fileclass.h"


namespace dal {

    struct AssimpModelInfo {
        binfo::Model m_model;
        std::vector<Animation> m_animations;
    };

    bool loadAssimpModel(const ResourceID& resID, AssimpModelInfo& info);

}
