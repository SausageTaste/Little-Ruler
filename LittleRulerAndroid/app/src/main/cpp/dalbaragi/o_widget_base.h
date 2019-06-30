#pragma once

#include <glm/glm.hpp>

#include "p_resource.h"


namespace dal {

    struct QuadInfo {
        glm::vec2 p1, p2;

        QuadInfo screen2device(void) const;
    };


    class IClickable {

    public:
        virtual ~IClickable(void) = default;
        virtual bool isInside(const float x, const float y) = 0;
        virtual void onClick(const float x, const float y) = 0;

    };


    class IKeyInputTaker {

    public:
        virtual ~IKeyInputTaker(void) = default;
        virtual void onKeyInput(const char* const c) = 0;

    };


    class ScreenQuad : public IClickable {

    public:
        enum class AlignMode {
            upper_left,
            upper_right
        };

    private:
        ScreenQuad* m_parent;
        QuadInfo m_deviceSpace;
        float m_xPos, m_yPos, m_width, m_height;
        AlignMode m_alignMode;

    public:
        ScreenQuad(void);

        virtual bool isInside(const float x, const float y) override;
        virtual void onClick(const float x, const float y) override {}

        void setParent(ScreenQuad* parent) { this->m_parent = parent; }

        float getPosX(void) const { return m_xPos; }
        float getPosY(void) const { return m_yPos; }
        float getWidth(void) const { return m_width; }
        float getHeight(void) const { return m_height; }

        void setPosX(const float v);
        void setPosY(const float v);
        void setWidth(const float v);
        void setHeight(const float v);
        void setAlignMode(const AlignMode mode);

        QuadInfo makeScreenSpace(void) const;
        const QuadInfo& getDeviceSpace(void) const;
        void onResize(const unsigned int width, const unsigned int height);

    private:
        void updateDeviceSpace(void);

    };


    class QuadRenderer {

    private:
        glm::vec4 m_color{ 1.0f, 1.0f, 1.0f, 1.0f };
        const Texture *m_diffuseMap = nullptr, *m_maskMap = nullptr;
        bool m_upsideDown_diffuseMap = false;
        bool m_upsideDown_maskMap = false;

    public:
        void setColor(const float r, const float g, const float b, const float a);
        void setDiffuseMap(const Texture* const tex) { m_diffuseMap = tex; }
        void setMaskMap(const Texture* const tex) { m_maskMap = tex; }
        void setUpsideDown_diffuseMap(const bool v);
        void setUpsideDown_maskMap(const bool v);

        void renderQuad(const UnilocOverlay& uniloc, const QuadInfo& devSpc);

        static void statelessRender(const UnilocOverlay& uniloc, const QuadInfo& devSpc, const glm::vec4& color,
            const Texture* const diffuseMap, const Texture* const maskMap,
            const bool upsideDown_diffuseMap, const bool upsideDown_maskMap
        );
    };


    class Widget : public ScreenQuad, public IKeyInputTaker {

    private:
        Widget* m_parent;
        bool m_isPauseOnly = true;

    public:
        explicit Widget(Widget* parent);
        virtual ~Widget(void) = default;
        virtual void renderOverlay(const UnilocOverlay& uniloc) = 0;
        virtual void onKeyInput(const char* const c) override {}
        virtual void onFocusChange(bool isFocus) {}

        void setPauseOnly(bool v) { this->m_isPauseOnly = v; }
        bool getPauseOnly(void) const { return this->m_isPauseOnly; }

    };

}