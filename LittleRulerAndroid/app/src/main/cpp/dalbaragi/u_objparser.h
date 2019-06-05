#pragma once

#include "u_loadinfo.h"
#include "u_fileclass.h"


namespace dal {

    bool loadAssimp_staticModel(loadedinfo::ModelStatic& info, ResourceID assetPath);

    bool loadAssimp_animatedModel(loadedinfo::ModelAnimated& info, ResourceID resID);

}