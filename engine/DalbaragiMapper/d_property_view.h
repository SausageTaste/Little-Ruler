#pragma once

#include <QWidget>

#include "d_scene.h"


namespace dal {

    class PropertyView : public QWidget {

    private:
        class Impl;
        Impl* m_pimpl = nullptr;

    public:
        PropertyView(QWidget* const parent, Scene& scene, SharedInfo& shared);
        ~PropertyView(void);

        void onSharedInfoUpdated(void);

    };

}
