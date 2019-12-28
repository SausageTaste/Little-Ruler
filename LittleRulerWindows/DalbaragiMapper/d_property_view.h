#pragma once

#include <vector>
#include <memory>

#include <QWidget>
#include <QLineEdit>
#include <QGridLayout>
#include <QDoubleSpinBox>

#include "d_scene.h"


namespace dal {

    class VectorSpinBox : public QWidget {

    private:
        class ConnectedSpinBox : public QDoubleSpinBox {

        private:
            fixed_t* m_target = nullptr;

        public:
            ConnectedSpinBox(QWidget* const parent);

            void setTarget(fixed_t* const target);

        };

    private:
        QGridLayout m_grid;
        std::vector<std::unique_ptr<QDoubleSpinBox>> m_spinBoxes;
        xvec3* m_target = nullptr;

    public:
        VectorSpinBox(const unsigned size);

        void setTarget(xvec3* const target);

    };


    class PropertyView : public QWidget {

    private:
        Scene& m_scene;
        SharedInfo& m_shared;

        QGridLayout m_grid;

        std::vector<std::unique_ptr<QLineEdit>> m_lineEdits;
        VectorSpinBox m_pos;

    public:
        PropertyView(QWidget* const parent, Scene& scene, SharedInfo& shared);

    };

}
