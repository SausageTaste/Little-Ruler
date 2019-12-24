#pragma once

#include <vector>
#include <memory>

#include <qwidget.h>
#include <qlineedit.h>
#include <qgridlayout.h>


namespace dal {

    class PropertyView : public QWidget {

    private:
        QGridLayout m_grid;

        std::vector<std::unique_ptr<QLineEdit>> m_lineEdits;

    public:
        PropertyView(QWidget* const parent);

    };

}
