#include "d_property_view.h"


namespace dal {

    PropertyView::PropertyView(QWidget* const parent)
        : QWidget(parent)
    {
        this->setLayout(&this->m_grid);

        for ( int i = 0; i < 5; ++i ) {
            this->m_lineEdits.emplace_back(new QLineEdit{ this });
            this->m_grid.addWidget(this->m_lineEdits.back().get());
        }
    }

}
