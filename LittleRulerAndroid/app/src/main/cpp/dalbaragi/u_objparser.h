#pragma once

#include "u_loadinfo.h"
#include "u_fileclass.h"
#include "p_model.h"


namespace dal {

    bool loadAssimp_staticModel(loadedinfo::ModelStatic& info, const ResourceID& assetPath);


    struct AssimpModelInfo {
        loadedinfo::ModelAnimated m_model;
        std::vector<Animation> m_animations;
    };

     bool loadAssimpModel(const ResourceID& resID, AssimpModelInfo& info);

}