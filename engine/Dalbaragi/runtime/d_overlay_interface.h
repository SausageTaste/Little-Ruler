#pragma once

#include <memory>

#include <d_widget.h>

#include "p_uniloc.h"
#include "p_meshStatic.h"


namespace dal {

    class OverlayTexInterf : public OverlayTexture {

    public:
        Texture m_tex;

    public:
        virtual void init_maskMap(const uint8_t* const image, const unsigned width, const unsigned height) override;

    };


    void drawOverlay(const OverlayDrawInfo& info, const void* const uniloc);
    bool loadFileBuf(const char* const respath, std::vector<uint8_t>& result);
    OverlayTexture* genOverlayTexture(void);

}
