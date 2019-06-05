#pragma once

#include "u_loadinfo.h"
#include "u_fileclass.h"


namespace dal {

    bool loadAssimp_staticModel(loadedinfo::ModelStatic& info, const ResourceID& assetPath);

    bool loadAssimp_animatedModel(loadedinfo::ModelAnimated& info, std::vector<dal::loadedinfo::Animation>& anims, const ResourceID& resID);

}