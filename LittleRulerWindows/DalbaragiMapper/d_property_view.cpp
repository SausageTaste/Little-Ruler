#include "d_property_view.h"

#include <functional>

#include <QLabel>
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
            this->m_grid.setAlignment(Qt::AlignLeft);

            for ( unsigned i = 0; i < _Size; ++i ) {
                QDoubleSpinBox& w = this->m_spinboxes.construct(i, this);
                this->m_grid.addWidget(&w, i, 0, 1, 1);
                const auto _ = connect(&w, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &VectorSpinBox::onValueChange);

                w.setMinimumWidth(80);

                w.setMinimum(-DBL_MAX);
                w.setSingleStep(0.01);
                w.setDecimals(3);
                w.setSuffix(" m");
            }
        }

        template <unsigned _Index>
        void setValue(const double v) {
            static_assert(_Index < _Size);
            QDoubleSpinBox& w = this->m_spinboxes[_Index];
            w.setValue(v);
        }
        template <unsigned _Index>
        double value(void) const {
            static_assert(_Index < _Size);
            QDoubleSpinBox& w = this->m_spinboxes[_Index];
            return w.value();
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

    class PropertyPage : public QWidget {

    protected:
        dal::SharedInfo& m_shared;
        QGridLayout m_grid;

    public:
        PropertyPage(const PropertyPage&) = delete;
        PropertyPage& operator=(const PropertyPage&) = delete;

    public:
        PropertyPage(QWidget* const parent, dal::SharedInfo& shared)
            : QWidget(parent)
            , m_shared(shared)
            , m_grid(this)
        {

        }
        virtual ~PropertyPage(void) = default;

        virtual void onSharedInfoUpdated(void) {}

    };


    class DataView_Actor : public PropertyPage {

    private:
        QLabel m_label_pos;
        VectorSpinBox<3> m_field_pos;

        QLabel m_label_quat;
        VectorSpinBox<4> m_field_quat;

        QLabel m_label_scale;
        VectorSpinBox<1> m_field_scale;

    public:
        DataView_Actor(QWidget* const parent, dal::SharedInfo& shared)
            : PropertyPage(parent, shared)

            , m_label_pos(this)
            , m_field_pos(this)
            , m_label_quat(this)
            , m_field_quat(this)
            , m_label_scale(this)
            , m_field_scale(this)
        {
            this->setLayout(&this->m_grid);
            this->m_grid.setAlignment(Qt::AlignTop);

            // Pos

            this->m_grid.addWidget(&this->m_label_pos, 0, 0, 1, 1);
            this->m_label_pos.setText("Pos");

            this->m_grid.addWidget(&this->m_field_pos, 0, 1, 1, 1);
            this->m_field_pos.register_onValueChange([this](const VectorSpinBox<3>::valueArray_t& arg) {
                auto trans = this->m_shared.m_active.m_trans;
                if ( nullptr != trans ) {
                    trans->setPos(arg[0], arg[1], arg[2]);
                    this->m_shared.m_needRedraw = true;
                }
                });

            // Quat

            this->m_grid.addWidget(&this->m_label_quat, 1, 0, 1, 1);
            this->m_label_quat.setText("Quat");

            this->m_grid.addWidget(&this->m_field_quat, 1, 1, 1, 1);
            this->m_field_quat.register_onValueChange([this](const VectorSpinBox<4>::valueArray_t& arg) {
                auto trans = this->m_shared.m_active.m_trans;
                if ( nullptr != trans ) {
                    trans->setQuat(arg[0], arg[1], arg[2], arg[3]);
                    this->m_shared.m_needRedraw = true;
                }
                });

            // Scale

            this->m_grid.addWidget(&this->m_label_scale, 2, 0, 1, 1);
            this->m_label_scale.setText("Scale");

            this->m_grid.addWidget(&this->m_field_scale, 2, 1, 1, 1);
            this->m_field_scale.register_onValueChange([this](const VectorSpinBox<1>::valueArray_t& arg) {
                auto trans = this->m_shared.m_active.m_trans;
                if ( nullptr != trans ) {
                    trans->setScale(arg[0]);
                    this->m_shared.m_needRedraw = true;
                }
                });
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
