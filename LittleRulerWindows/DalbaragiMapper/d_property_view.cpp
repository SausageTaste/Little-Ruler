#include "d_property_view.h"


namespace dal {

    VectorSpinBox::ConnectedSpinBox::ConnectedSpinBox(QWidget* const parent)
        : QDoubleSpinBox(parent)
    {

    }

    void VectorSpinBox::ConnectedSpinBox::setTarget(fixed_t* const target) {
        this->m_target = target;
    }


    VectorSpinBox::VectorSpinBox(const unsigned size)
        : m_grid(this)
    {
        this->setLayout(&this->m_grid);

        this->m_spinBoxes.reserve(size);
        for ( unsigned i = 0; i < size; ++i ) {
            auto& w = this->m_spinBoxes.emplace_back(new QDoubleSpinBox{ this });
            this->m_grid.addWidget(w.get(), 0, i);
        }
    }

    void VectorSpinBox::setTarget(xvec3* const target) {
        this->m_target = target;
    }

}


namespace dal {

    PropertyView::PropertyView(QWidget* const parent, Scene& scene, SharedInfo& shared)
        : QWidget(parent)
        , m_scene(scene)
        , m_shared(shared)
        , m_pos(3)
    {
        this->setLayout(&this->m_grid);

        for ( int i = 0; i < 5; ++i ) {
            this->m_lineEdits.emplace_back(new QLineEdit{ this });
            this->m_grid.addWidget(this->m_lineEdits.back().get());
        }

        this->m_grid.addWidget(&this->m_pos);
    }

}
