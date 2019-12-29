#pragma once

#include <QOpenGLWidget>
#include <QMouseEvent>

#include "d_opengl_state.h"
#include "d_scene.h"
#include "d_time.h"
#include "d_sharedinfo.h"


namespace dal {

    class GraphicsView : public QOpenGLWidget {

    private:
        gl::State& m_glstate;
        Scene& m_scene;
        SharedInfo& m_shared;

        Timer m_timer;
        int m_timerID;
        glm::mat4 m_projMat;

    public:
        GraphicsView(QWidget* const parent, gl::State& glstate, Scene& scene, SharedInfo& shared);
        ~GraphicsView(void);

        virtual void initializeGL(void) override;
        virtual void resizeGL(int w, int h) override;
        virtual void paintGL(void) override;

        virtual void mousePressEvent(QMouseEvent* e) override;
        virtual void mouseReleaseEvent(QMouseEvent* e) override;
        virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
        virtual void mouseMoveEvent(QMouseEvent* e) override;

        virtual void keyPressEvent(QKeyEvent* e) override;
        virtual void keyReleaseEvent(QKeyEvent* e) override;

        virtual void focusInEvent(QFocusEvent* event) override;
        virtual void focusOutEvent(QFocusEvent* event) override;

        virtual void timerEvent(QTimerEvent* event) override;

    private:
        bool applyKeyMove(const double deltaTime);

        void startLoop(void);
        void killLoop(void);

    };

}
