#include "d_graphics_view.h"

#include <array>
#include <chrono>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include <u_fileutils.h>
#include <d_logger.h>

#include "d_opengl.h"
#include "d_meshgeo.h"
#include "d_time.h"


namespace {

    class MouseState {

    public:
        bool m_holding = false;
        glm::ivec2 m_downPos{ 0 }, m_lastMovePos{ 0 };

    } g_mouse;


    class KeyState {

    public:
        enum class KeySpec {
            w, a, s, d,
            space, lshift,
            eoe
        };

        constexpr static int NUM_KEY_SPEC = static_cast<int>(KeySpec::eoe) - static_cast<int>(KeySpec::w);

    public:
        std::array<bool, NUM_KEY_SPEC> m_isDown = { false };

    public:
        KeySpec mapFromQKey(const int key) {
            switch ( key ) {

            case Qt::Key_W:
                return KeySpec::w;
            case Qt::Key_S:
                return KeySpec::s;
            case Qt::Key_A:
                return KeySpec::a;
            case Qt::Key_D:
                return KeySpec::d;

            case Qt::Key_Space:
                return KeySpec::space;
            case Qt::Key_Shift:
                return KeySpec::lshift;

            default:
                return KeySpec::eoe;

            }
        }

        void clearDowns(void) {
            for ( int i = 0; i < this->NUM_KEY_SPEC; ++i ) {
                this->m_isDown[i] = false;
            }

            dalInfo("Key states cleared");
        }

        bool isDown(const KeySpec key) const {
            return this->m_isDown[static_cast<int>(key)];
        }

        void setDownOrIgnore(const KeySpec key, const bool down) {
            const auto index = static_cast<unsigned int>(key);
            if ( index < this->NUM_KEY_SPEC ) {
                this->m_isDown[index] = down;
            }
        }

    } g_key;

}


namespace dal {

    GraphicsView::GraphicsView(QWidget* const parent, gl::State& glstate, Scene& scene, SharedInfo& shared)
        : QOpenGLWidget(parent)
        , m_glstate(glstate)
        , m_scene(scene)
        , m_shared(shared)
        , m_timerID(-1)
        , m_projMat(1.f)
    {
        this->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    }

    GraphicsView::~GraphicsView(void) {
        this->makeCurrent();
        this->m_scene.clearGL();
    }


    void GraphicsView::initializeGL(void) {
        gl::setClearColor(0.f, 0.f, 0.f, 1.f);
        this->m_glstate.init();

        {
            constexpr float a = 1.f;

            auto& mesh = this->m_scene.addMesh("one");
            mesh.m_meshdata.addQuad({ -a, a, 0 }, { -a, -a, 0 }, { a, -a, 0 }, { a, a, 0 });
            const auto build = mesh.m_meshdata.buildMesh();
            mesh.m_glmesh.initStatic(build.numVert(), build.vertices(), build.texcoords(), build.normals());

            this->m_shared.m_active.m_actor = &mesh.m_actor;
            this->notify_onSharedInfoUpdated();
        }

        {
            ImageData image;
            const auto res = loadFileImage("asset::rustediron2_basecolor.png", image);
            assert(res);
            this->m_scene.m_albedo.init_image(image);
        }

        {
            ImageData image;
            const auto res = loadFileImage("asset::rustediron2_roughness.png", image);
            assert(res);
            this->m_scene.m_roughness.init_image(image);
        }

        {
            ImageData image;
            const auto res = loadFileImage("asset::rustediron2_metallic.png", image);
            assert(res);
            this->m_scene.m_metallic.init_image(image);
        }

        this->startLoop();

        assert(gl::queryVersion().first >= 3);
    }

    void GraphicsView::resizeGL(int w, int h) {
        const auto aspectRatio = static_cast<float>(this->width()) / static_cast<float>(this->height());
        this->m_projMat = glm::perspective(90.f, aspectRatio, 0.1f, 100.f);
        this->m_glstate.setViewportSize(this->width(), this->height());
    }

    void GraphicsView::paintGL(void) {
        Timer timer;

        gl::clear(gl::ClearMode::color);

        auto& uniloc = this->m_glstate.use_static();
        uniloc.u_projMat << this->m_projMat;
        this->m_scene.render(uniloc);

        dalInfo(fmt::format("Painted for {:.4f} ms", timer.getElapsed() * 1000.0));
    }


    void GraphicsView::mousePressEvent(QMouseEvent* e) {
        g_mouse.m_holding = true;
        g_mouse.m_downPos.x = e->globalX();
        g_mouse.m_downPos.y = e->globalY();
        g_mouse.m_lastMovePos = g_mouse.m_downPos;
    }

    void GraphicsView::mouseReleaseEvent(QMouseEvent* e) {
        g_mouse.m_holding = false;
    }

    void GraphicsView::mouseDoubleClickEvent(QMouseEvent* e) {

    }

    void GraphicsView::mouseMoveEvent(QMouseEvent* e) {
        const glm::ivec2 thisPos{ e->globalX(), e->globalY() };
        const auto rel = thisPos - g_mouse.m_lastMovePos;

        const double multiplier = 2.0 / static_cast<double>(this->height());

        this->m_scene.activeCam().rotateHorizontal(multiplier * rel.x);
        this->m_scene.activeCam().rotateVertical(multiplier * rel.y);
        g_mouse.m_lastMovePos = thisPos;

        this->update();
    }

    void GraphicsView::keyPressEvent(QKeyEvent* e) {
        const auto key = g_key.mapFromQKey(e->key());
        g_key.setDownOrIgnore(key, true);
    }

    void GraphicsView::keyReleaseEvent(QKeyEvent* e) {
        const auto key = g_key.mapFromQKey(e->key());
        g_key.setDownOrIgnore(key, false);
    }

    void GraphicsView::focusInEvent(QFocusEvent* event) {
        g_key.clearDowns();
    }

    void GraphicsView::focusOutEvent(QFocusEvent* event) {
        g_key.clearDowns();
    }

    void GraphicsView::timerEvent(QTimerEvent* event) {
        bool needUpdate = false;

        const auto moved = this->applyKeyMove(this->m_timer.checkGetElapsed());
        if ( moved ) {
            needUpdate = true;
        }

        if ( this->m_shared.m_needRedraw ) {
            needUpdate = true;
            this->m_shared.m_needRedraw = false;
        }

        if ( needUpdate )
            this->update();
    }

    // Private

    bool GraphicsView::applyKeyMove(const double deltaTime) {
        glm::vec2 moveDir{ 0.f };
        int vertical = 0;

        if ( g_key.isDown(KeyState::KeySpec::w) ) {
            moveDir.y -= 1.f;
        }
        if ( g_key.isDown(KeyState::KeySpec::s) ) {
            moveDir.y += 1.f;
        }
        if ( g_key.isDown(KeyState::KeySpec::a) ) {
            moveDir.x -= 1.f;
        }
        if ( g_key.isDown(KeyState::KeySpec::d) ) {
            moveDir.x += 1.f;
        }

        if ( g_key.isDown(KeyState::KeySpec::space) ) {
            vertical += 1;
        }
        if ( g_key.isDown(KeyState::KeySpec::lshift) ) {
            vertical -= 1;
        }

        const auto lengthSqr = glm::dot(moveDir, moveDir);
        if ( lengthSqr > 0.1f ) {
            const auto moveDir_n = moveDir / sqrt(lengthSqr);
            const glm::vec3 newvec{ moveDir_n.x, vertical, moveDir_n.y };
            this->m_scene.activeCam().moveHorizontalAlongView(newvec * static_cast<float>(deltaTime));
            return true;
        }
        else {
            if ( 0 != vertical ) {
                const glm::vec3 newvec{ 0, vertical, 0 };
                this->m_scene.activeCam().moveHorizontalAlongView(newvec * static_cast<float>(deltaTime));
                return true;
            }
        }

        return false;
    }

    void GraphicsView::startLoop(void) {
        if ( -1 == this->m_timerID ) {
            constexpr double FPS = 60.0;
            this->m_timerID = this->startTimer(1000.0 / FPS);
            this->m_timer.check();
        }
    }

    void GraphicsView::killLoop(void) {
        if ( -1 != this->m_timerID ) {
            this->killTimer(this->m_timerID);
            this->m_timerID = -1;
        }
    }

    void GraphicsView::notify_onSharedInfoUpdated(void) {
        if ( this->m_funcOnSharedInfoUpdated ) {
            this->m_funcOnSharedInfoUpdated();
        }
        else {
            dalWarn("GraphicsView::m_funcOnSharedInfoUpdated is null.");
        }
    }

}
