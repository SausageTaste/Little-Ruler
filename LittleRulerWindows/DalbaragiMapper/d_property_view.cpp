#include "d_property_view.h"

#include <iostream>
#include <functional>

#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QDoubleSpinBox>

#include <d_pool.h>
#include <d_logger.h>


namespace {

    template <unsigned _Size>
    class VectorSpinBox : public QWidget {

    public:
        using valueArray_t = std::array<double, _Size>;
        using callbackOnValueChange_t = std::function<void(const valueArray_t&)>;

    private:
        static constexpr QSizePolicy SIZE_POLICY{ QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed };

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
            this->m_grid.setMargin(0);

            for ( unsigned i = 0; i < _Size; ++i ) {
                QDoubleSpinBox& w = this->m_spinboxes.construct(i, this);
                this->m_grid.addWidget(&w, i, 0, 1, 1);
                const auto _ = connect(&w, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &VectorSpinBox::onValueChange);

                w.setMinimumWidth(80);
                w.setMaximumWidth(99999);
                w.setSizePolicy(SIZE_POLICY);

                w.setMinimum(-DBL_MAX);
                w.setMaximum(DBL_MAX);
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
        bool m_isConnectedSlotDisabled = false;

        QLabel m_label_name;
        QLineEdit m_lineEdit_name;

        QLabel m_label_pos;
        VectorSpinBox<3> m_field_pos;

        QLabel m_label_quat;
        VectorSpinBox<4> m_field_quat;

        QLabel m_label_scale;
        VectorSpinBox<1> m_field_scale;

    public:
        DataView_Actor(QWidget* const parent, dal::SharedInfo& shared)
            : PropertyPage(parent, shared)

            , m_label_name(this)
            , m_lineEdit_name(this)
            , m_label_pos(this)
            , m_field_pos(this)
            , m_label_quat(this)
            , m_field_quat(this)
            , m_label_scale(this)
            , m_field_scale(this)
        {
            this->setLayout(&this->m_grid);
            this->m_grid.setAlignment(Qt::AlignTop);

            // Name
            {
                this->m_grid.addWidget(&this->m_label_name, 0, 0, 1, 1);
                this->m_label_name.setText("Name");

                this->m_grid.addWidget(&this->m_lineEdit_name, 0, 1, 1, 1);
                const auto _ = connect(&this->m_lineEdit_name,
                    static_cast<void(QLineEdit::*)(const QString&)>(&QLineEdit::textChanged),
                    this,
                    &DataView_Actor::onNameEditted
                );
            }

            // Pos
            {
                this->m_grid.addWidget(&this->m_label_pos, 1, 0, 1, 1);
                this->m_label_pos.setText("Pos");
                QSizePolicy policy;
                policy.setHorizontalPolicy(QSizePolicy::Policy::Fixed);
                this->m_label_pos.setSizePolicy(policy);

                this->m_grid.addWidget(&this->m_field_pos, 1, 1, 1, 1);
                this->m_field_pos.register_onValueChange([this](const VectorSpinBox<3>::valueArray_t& arg) {
                    if ( this->m_isConnectedSlotDisabled )
                        return;

                    auto actor = this->m_shared.m_active.m_actor;
                    if ( nullptr != actor ) {
                        actor->m_trans.setPos(arg[0], arg[1], arg[2]);
                        this->m_shared.m_needRedraw = true;
                    }
                    });
            }

            // Quat
            {
                this->m_grid.addWidget(&this->m_label_quat, 2, 0, 1, 1);
                this->m_label_quat.setText("Quat");

                this->m_grid.addWidget(&this->m_field_quat, 2, 1, 1, 1);
                this->m_field_quat.register_onValueChange([this](const VectorSpinBox<4>::valueArray_t& arg) {
                    if ( this->m_isConnectedSlotDisabled )
                        return;

                    auto actor = this->m_shared.m_active.m_actor;
                    if ( nullptr != actor ) {
                        actor->m_trans.setQuat(arg[1], arg[2], arg[3], arg[0]);
                        this->m_shared.m_needRedraw = true;
                    }
                    });
            }

            // Scale
            {
                this->m_grid.addWidget(&this->m_label_scale, 3, 0, 1, 1);
                this->m_label_scale.setText("Scale");

                this->m_grid.addWidget(&this->m_field_scale, 3, 1, 1, 1);
                this->m_field_scale.register_onValueChange([this](const VectorSpinBox<1>::valueArray_t& arg) {
                    if ( this->m_isConnectedSlotDisabled )
                        return;

                    auto actor = this->m_shared.m_active.m_actor;
                    if ( nullptr != actor ) {
                        actor->m_trans.setScale(arg[0]);
                        this->m_shared.m_needRedraw = true;
                    }
                    });
            }
        }

        virtual void onSharedInfoUpdated(void) override {
            const auto actor = this->m_shared.m_active.m_actor;
            if ( nullptr == actor )
                return;

            this->m_isConnectedSlotDisabled = true;

            this->m_lineEdit_name.setText(actor->m_name.c_str());

            this->m_field_pos.setValue<0>(actor->m_trans.pos().x);
            this->m_field_pos.setValue<1>(actor->m_trans.pos().y);
            this->m_field_pos.setValue<2>(actor->m_trans.pos().z);

            this->m_field_quat.setValue<0>(actor->m_trans.quat().w);
            this->m_field_quat.setValue<1>(actor->m_trans.quat().x);
            this->m_field_quat.setValue<2>(actor->m_trans.quat().y);
            this->m_field_quat.setValue<3>(actor->m_trans.quat().z);

            this->m_field_scale.setValue<0>(actor->m_trans.scale());

            this->m_isConnectedSlotDisabled = false;
        }

    private:
        void onNameEditted(const QString& str) {
            const auto actor = this->m_shared.m_active.m_actor;
            if ( nullptr != actor ) {
                actor->m_name = str.toStdString();
                dalVerbose(actor->m_name);
            }
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

    void PropertyView::onSharedInfoUpdated(void) {
        this->m_pimpl->m_view_actor.onSharedInfoUpdated();
    }

}
