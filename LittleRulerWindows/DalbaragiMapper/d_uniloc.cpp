#include "d_uniloc.h"


namespace dal::gl {

    void UniInterf_Lights::init(const Shader& shader) {
        using namespace std;

        u_baseAmbient >> shader.getUniloc("u_baseAmbient");

        u_dlightCount >> shader.getUniloc("u_dlightCount");
        u_plightCount >> shader.getUniloc("u_plightCount");
        u_slightCount >> shader.getUniloc("u_slightCount");

        for ( unsigned i = 0; i < MAX_NUM_DLIGHT; ++i ) {
            const auto index = "["s + to_string(i) + ']';
            u_dlight_direcs[i] >> shader.getUniloc("u_dlight_direcs"s + index);
            u_dlight_colors[i] >> shader.getUniloc("u_dlight_colors"s + index);
        }

        
        for ( unsigned i = 0; i < MAX_NUM_PLIGHT; ++i ) {
            const auto index = "["s + to_string(i) + ']';
            u_plight_poses[i] >> shader.getUniloc("u_plight_poses"s + index);
            u_plight_colors[i] >> shader.getUniloc("u_plight_colors"s + index);
        }

        for ( unsigned i = 0; i < MAX_NUM_SLIGHT; ++i ) {
            const auto index = "["s + to_string(i) + ']';
            u_slight_poses[i] >> shader.getUniloc("u_slight_poses"s + index);
            u_slight_direcs[i] >> shader.getUniloc("u_slight_direcs"s + index);
            u_slight_colors[i] >> shader.getUniloc("u_slight_colors"s + index);
            u_slight_fadeStart[i] >> shader.getUniloc("u_slight_fadeStart"s + index);
            u_slight_fadeEnd[i] >> shader.getUniloc("u_slight_fadeEnd"s + index);
        }
    }

    void Uniloc_Static::init(const Shader& shader) {
        this->u_projMat >> shader.getUniloc("u_projMat");
    }

}
