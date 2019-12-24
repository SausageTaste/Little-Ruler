#include "d_dalbaragimapper.h"

#include <iostream>

#include <glm/glm.hpp>


namespace {

    bool isInside(const QWidget& w, const int x, const int y) {
        const auto globalPos = w.mapToGlobal(w.pos());
        const auto xmin = globalPos.x();
        const auto ymin = globalPos.y();
        const auto xmax = xmin + w.width();
        const auto ymax = ymin + w.height();

        if ( x < xmin ) {
            return false;
        }
        else if ( x > xmax ) {
            return false;
        }
        else if ( y < ymin ) {
            return false;
        }
        else if ( y > ymax ) {
            return false;
        }
        else {
            return true;
        }
    }

}


namespace dal {

    MainWidget::MainWidget(QWidget* const parent)
        : QWidget(parent)
        , m_layout(this)
        , m_propertyView(this)
        , m_graphicsView(this, m_glstate, m_scene)
    {
        this->setLayout(&this->m_layout);

        this->m_layout.addWidget(&this->m_splitter);

        this->m_splitter.addWidget(&this->m_graphicsView);
        this->m_splitter.addWidget(&this->m_propertyView);

        this->m_scene.activeCam().m_pos.z = 1;
    }

    void MainWidget::mousePressEvent(QMouseEvent* e) {

    }

    void MainWidget::mouseReleaseEvent(QMouseEvent* e) {

    }

    void MainWidget::mouseDoubleClickEvent(QMouseEvent* e) {

    }

    void MainWidget::mouseMoveEvent(QMouseEvent* e) {

    }

}


namespace dal {

    DalbaragiMapper::DalbaragiMapper(void)
        : QMainWindow(nullptr)
        , m_mainWidget(this)
    {
        this->setCentralWidget(&this->m_mainWidget);

        this->setMinimumSize(800, 450);
    }

}
