#include "d_property_view.h"

#include <functional>

#include <QGridLayout>
#include <QDoubleSpinBox>

#include <d_pool.h>


namespace {
    
    template <unsigned _Size>
    class VectorSpinBox : public QWidget {

    public:
        using valueArray_t = std::array<double, _Size>;
        using callbackOnValueChange_t = std::function<void(const valueArray_t&)>;

    private:
        QGridLayout m_grid;
        dal::NoInitArray<QDoubleSpinBox, _Size> m_spinboxes;
        callbackOnValueChange_t m_onValueChange;

    public:
        VectorSpinBox(QWidget* const parent)
            : QWidget(parent)
            , m_grid(this)
        {
            this->setLayout(&this->m_grid);

            for ( unsigned i = 0; i < _Size; ++i ) {
                QDoubleSpinBox& w = this->m_spinboxes.construct(i, this);
                this->m_grid.addWidget(&w, 0, i, 1, 1);
                const auto _ = connect(&w, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &VectorSpinBox::onValueChange);

                w.setMinimum(-DBL_MAX);
                w.setSingleStep(0.01);
                w.setDecimals(4);
            }
        }

        void register_onValueChange(callbackOnValueChange_t func) {
            this->m_onValueChange = func;
        }

    private:
        void onValueChange(const double x) {
            if ( this->m_onValueChange ) {
                valueArray_t arg;
                for ( unsigned i = 0; i < _Size; ++i ) {
                    arg[i] = this->m_spinboxes[i].value();
                }

                this->m_onValueChange(arg);
            }
        }

    };

}


namespace {

    class DataView_Actor : public QWidget {

    private:
        dal::SharedInfo& m_shared;

        QGridLayout m_grid;
        VectorSpinBox<3> m_field_pos;

    public:
        DataView_Actor(QWidget* const parent, dal::SharedInfo& shared)
            : QWidget(parent)
            , m_shared(shared)
            , m_grid(this)
            , m_field_pos(this)
        {
            this->setLayout(&this->m_grid);

            this->m_grid.addWidget(&this->m_field_pos);

            auto a = [this](const VectorSpinBox<3>::valueArray_t& arg) {
                auto trans = this->m_shared.m_active.m_trans;
                if ( nullptr != trans ) {
                    trans->setPos(arg[0], arg[1], arg[2]);
                    this->m_shared.m_needRedraw = true;
                }
            };
            this->m_field_pos.register_onValueChange(a);
        }

    };

}


namespace dal {

    class PropertyView::Impl {

    public:
        Scene& m_scene;
        SharedInfo& m_shared;

        QGridLayout m_grid;

        DataView_Actor m_view_actor;

    public:
        Impl(QWidget* const parent, Scene& scene, SharedInfo& shared)
            : m_scene(scene)
            , m_shared(shared)
            , m_grid(parent)
            , m_view_actor(parent, shared)
        {

        }

    };


    PropertyView::PropertyView(QWidget* const parent, Scene& scene, SharedInfo& shared)
        : QWidget(parent)
        , m_pimpl(new Impl{ this, scene, shared })
    {
        this->setLayout(&this->m_pimpl->m_grid);

        this->m_pimpl->m_grid.addWidget(&this->m_pimpl->m_view_actor);
    }

    PropertyView::~PropertyView(void) {
        delete this->m_pimpl;
        this->m_pimpl = nullptr;
    }

}
