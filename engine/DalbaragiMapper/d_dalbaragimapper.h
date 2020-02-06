#pragma once

#include <QtWidgets/QMainWindow>
#include <qwidget.h>
#include <qgridlayout.h>
#include <qsplitter.h>
#include <QMouseEvent>

#include "d_property_view.h"
#include "d_graphics_view.h"


namespace dal {

    class MainWidget : public QWidget {

    private:
        QGridLayout m_layout;
        QSplitter m_splitter;

        gl::State m_glstate;
        Scene m_scene;
        SharedInfo m_shared;

        PropertyView m_propertyView;
        GraphicsView m_graphicsView;

    public:
        MainWidget(QWidget* const parent);

    };


    class DalbaragiMapper : public QMainWindow {

    private:
        MainWidget m_mainWidget;

    public:
        DalbaragiMapper(void);

    };

}
