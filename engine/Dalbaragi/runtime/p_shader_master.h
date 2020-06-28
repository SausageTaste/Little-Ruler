#pragma once

#include "p_uniloc.h"


namespace dal {

    class ShaderLoader;

    class ShaderProgram {

    private:
        GLuint m_id = 0;

    public:
        ShaderProgram(void) = default;
        ShaderProgram(const std::string& vertSrc, const std::string& fragSrc);

        void init(const char* const vertSrc, const char* const fragSrc);
        void init(const std::string& vertSrc, const std::string& fragSrc);

        GLuint get(void) const;
        void use(void) const;

    };


    class ShaderMaster {

    private:
        ShaderProgram m_static;
        UniRender_Static u_static;

        ShaderProgram m_animated;
        UniRender_Animated u_animated;

        ShaderProgram m_static_depth;
        UniRender_StaticDepth u_static_depth;

        ShaderProgram m_animatedDepth;
        UniRender_AnimatedDepth u_animatedDepth;

        ShaderProgram m_static_onWater;
        UniRender_StaticOnWater u_static_onWater;

        ShaderProgram m_animated_onWater;
        UniRender_AnimatedOnWater u_animated_onWater;

        ShaderProgram m_fillScreen;
        UniRender_FillScreen u_fillScreen;

        ShaderProgram m_water;
        UniRender_Water u_water;

        ShaderProgram m_skybox;
        UniRender_Skybox u_skybox;

        ShaderProgram m_overlay;
        UniRender_Overlay u_overlay;

        ShaderProgram m_cube_irradiance;
        UniRender_CubeIrradiance u_cube_irradiance;

        ShaderProgram m_cube_prefilter;
        UniRender_CubePrefilter u_cube_prefilter;

        ShaderProgram m_brdfLUT;

    public:
        ShaderMaster(void);

        auto useStatic(void) const -> const UniRender_Static&;
        auto useAnimated(void) const -> const UniRender_Animated&;
        auto useStaticDepth(void) const -> const UniRender_StaticDepth&;
        auto useAnimatedDepth(void) const -> const UniRender_AnimatedDepth&;
        auto useStaticOnWater(void) const -> const UniRender_StaticOnWater&;
        auto useAnimatedOnWater(void) const -> const UniRender_AnimatedOnWater&;
        auto useFillScreen(void) const -> const UniRender_FillScreen&;
        auto useWater(void) const -> const UniRender_Water&;
        auto useSkybox(void) const -> const UniRender_Skybox&;
        auto useOverlay(void) const -> const UniRender_Overlay&;
        auto useCubeIrradiance(void) const -> const UniRender_CubeIrradiance&;
        auto useCubePrefilter(void) const -> const UniRender_CubePrefilter&;
        void useBrdfLUT(void) const;

    };

}
