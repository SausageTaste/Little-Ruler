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
        ShaderProgram m_overlay;
        UnilocOverlay m_overlayUniloc;

        ShaderProgram m_skybox;
        UnilocSkybox m_skyboxUniloc;

        //

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

    public:
        ShaderMaster(void);

        const UnilocOverlay& useOverlay(void) const;
        const UnilocSkybox& useSkybox(void) const;

        const UniRender_Static& useStatic(void) const;
        const UniRender_Animated& useAnimated(void) const;
        const UniRender_StaticDepth& useStaticDepth(void) const;
        const UniRender_AnimatedDepth& useAnimatedDepth(void) const;
        const UniRender_StaticOnWater& useStaticOnWater(void) const;
        const UniRender_AnimatedOnWater& useAnimatedOnWater(void) const;
        const UniRender_FillScreen& useFillScreen(void) const;
        const UniRender_Water& useWater(void) const;

    };

}
