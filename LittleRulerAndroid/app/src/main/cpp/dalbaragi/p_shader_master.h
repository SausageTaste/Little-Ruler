#pragma once

#include "p_uniloc.h"


namespace dal {

    class ShaderLoader;

    class ShaderProgram {

    private:
        GLuint m_id = 0;

    public:
        ShaderProgram(const char* const vertSrc, const char* const fragSrc);
        ShaderProgram(const std::string& vertSrc, const std::string& fragSrc);
        GLuint get(void);
        void use(void) const;

    };


    class ShaderMaster {

    private:
        ShaderProgram m_general;
        UnilocGeneral m_generalUniloc;

        ShaderProgram m_fscreen;
        UnilocFScreen m_fscreenUniloc;

        ShaderProgram m_depthmap;
        UnilocDepthmp m_depthmapUniloc;

        ShaderProgram m_overlay;
        UnilocOverlay m_overlayUniloc;

        ShaderProgram m_waterry;
        UnilocWaterry m_waterryUniloc;

        ShaderProgram m_animate;
        UnilocAnimate m_animateUniloc;

        ShaderProgram m_depthAnime;
        UnilocDepthAnime m_depthAnimeUniloc;

        ShaderProgram m_skybox;
        UnilocSkybox m_skyboxUniloc;

        //

        ShaderProgram m_static;
        UniRender_Static u_static;

        ShaderProgram m_animated;
        UniRender_Animated u_animated;

    public:
        ShaderMaster(void);

        const UnilocGeneral& useGeneral(void) const;
        const UnilocDepthmp& useDepthMp(void) const;
        const UnilocFScreen& useFScreen(void) const;
        const UnilocOverlay& useOverlay(void) const;
        const UnilocWaterry& useWaterry(void) const;
        const UnilocAnimate& useAnimate(void) const;
        const UnilocDepthAnime& useDepthAnime(void) const;
        const UnilocSkybox& useSkybox(void) const;

        const UniRender_Static& useStatic(void) const;
        const UniRender_Animated& useAnimated(void) const;

    };

}
