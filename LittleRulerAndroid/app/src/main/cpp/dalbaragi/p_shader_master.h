#pragma once

#include "p_uniloc.h"


namespace dal {

    class ShaderLoader;

    class ShaderProgram2 {

    private:
        GLuint m_id = 0;

    public:
        ShaderProgram2(const char* const vertSrc, const char* const fragSrc);
        ShaderProgram2(const std::string& vertSrc, const std::string& fragSrc);
        GLuint get(void);
        void use(void) const;

    };


    class ShaderMaster {

    private:
        ShaderProgram2 m_general;
        UnilocGeneral m_generalUniloc;

        ShaderProgram2 m_depthmap;
        UnilocDepthmp m_depthmapUniloc;

        ShaderProgram2 m_fscreen;
        UnilocFScreen m_fscreenUniloc;

        ShaderProgram2 m_overlay;
        UnilocOverlay m_overlayUniloc;

        ShaderProgram2 m_waterry;
        UnilocWaterry m_waterryUniloc;

        ShaderProgram2 m_animate;
        UnilocAnimate m_animateUniloc;

        ShaderProgram2 m_depthAnime;
        UnilocDepthAnime m_depthAnimeUniloc;

    public:
        ShaderMaster(void);

        const UnilocGeneral& useGeneral(void) const;
        const UnilocDepthmp& useDepthMp(void) const;
        const UnilocFScreen& useFScreen(void) const;
        const UnilocOverlay& useOverlay(void) const;
        const UnilocWaterry& useWaterry(void) const;
        const UnilocAnimate& useAnimate(void) const;
        const UnilocDepthAnime& useDepthAnime(void) const;

    };

}