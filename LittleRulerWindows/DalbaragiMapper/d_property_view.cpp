#include "d_property_view.h"

#include <memory>

#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QComboBox>

#include <d_pool.h>


namespace {

    // From https://stackoverflow.com/questions/5395266/removing-widgets-from-qgridlayout
    class GridLayoutUtil {
        /**
         * Utility class to remove the contents of a QGridLayout row, column or
         * cell. If the deleteWidgets parameter is true, then the widgets become
         * not only removed from the layout, but also deleted. Note that we won't
         * actually remove any row or column itself from the layout, as this isn't
         * possible. So the rowCount() and columnCount() will always stay the same,
         * but the contents of the row, column or cell will be removed.
         */

    public:
        // Removes the contents of the given layout row.
        static void removeRow(QGridLayout* layout, int row, bool deleteWidgets = true) {
            remove(layout, row, -1, deleteWidgets);
            layout->setRowMinimumHeight(row, 0);
            layout->setRowStretch(row, 0);
        }

        // Removes the contents of the given layout column.
        static void removeColumn(QGridLayout* layout, int column, bool deleteWidgets = true) {
            remove(layout, -1, column, deleteWidgets);
            layout->setColumnMinimumWidth(column, 0);
            layout->setColumnStretch(column, 0);
        }

        // Removes the contents of the given layout cell.
        static void removeCell(QGridLayout* layout, int row, int column, bool deleteWidgets = true) {
            remove(layout, row, column, deleteWidgets);
        }

    private:
        // Removes all layout items which span the given row and column.
        static void remove(QGridLayout* layout, int row, int column, bool deleteWidgets) {
            // We avoid usage of QGridLayout::itemAtPosition() here to improve performance.
            for ( int i = layout->count() - 1; i >= 0; i-- ) {
                int r, c, rs, cs;
                layout->getItemPosition(i, &r, &c, &rs, &cs);
                if (
                    (row == -1 || (r <= row && r + rs > row)) &&
                    (column == -1 || (c <= column && c + cs > column)) ) {
                    // This layout item is subject to deletion.
                    QLayoutItem* item = layout->takeAt(i);
                    if ( deleteWidgets ) {
                        deleteChildWidgets(item);
                    }
                    delete item;
                }
            }
        }

        // Deletes all child widgets of the given layout item.
        static void deleteChildWidgets(QLayoutItem* item) {
            QLayout* layout = item->layout();
            if ( layout ) {
                // Process all child items recursively.
                int itemCount = layout->count();
                for ( int i = 0; i < itemCount; i++ ) {
                    deleteChildWidgets(layout->itemAt(i));
                }
            }
            delete item->widget();
        }

    };

}


namespace {

    class PropertyPage : public QWidget {

    protected:
        dal::Scene& m_scene;
        dal::SharedInfo& m_shared;
        QGridLayout m_grid;

    public:
        PropertyPage(const PropertyPage&) = delete;
        PropertyPage& operator=(const PropertyPage&) = delete;

    public:
        PropertyPage(QWidget* const parent, dal::Scene& scene, dal::SharedInfo& shared)
            : QWidget(parent)
            , m_scene(scene)
            , m_shared(shared)
            , m_grid(this)
        {
            this->m_grid.setAlignment(Qt::AlignTop);
        }
        virtual ~PropertyPage(void) = default;

        virtual void onSharedInfoUpdated(void) {}

    };


    class DataView_Actor : public PropertyPage {

    private:
        static constexpr unsigned NUM_DOUBLE_VALUE = 3 + 4 + 1;
        static constexpr QSizePolicy LABEL_SIZE_POLICY{ QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed };

    private:
        bool m_slotDisabled = false;

        QLabel m_label_name;
        QLineEdit m_lineEdit_name;

        dal::NoInitArray<QDoubleSpinBox, NUM_DOUBLE_VALUE> m_dspinboxes;
        dal::NoInitArray<QLabel, NUM_DOUBLE_VALUE> m_labels_forDspinboxes;

    public:
        DataView_Actor(QWidget* const parent, dal::Scene& scene, dal::SharedInfo& shared)
            : PropertyPage(parent, scene, shared)

            , m_label_name(this)
            , m_lineEdit_name(this)
        {
            this->setLayout(&this->m_grid);

            // Name
            {
                this->m_grid.addWidget(&this->m_label_name, 0, 0, 1, 1, Qt::AlignRight);
                this->m_label_name.setText("Name");
                this->m_label_name.setSizePolicy(this->LABEL_SIZE_POLICY);

                this->m_grid.addWidget(&this->m_lineEdit_name, 0, 1, 1, 1);
                const auto _ = connect(
                    &this->m_lineEdit_name, static_cast<void(QLineEdit::*)(const QString&)>(&QLineEdit::textChanged),
                    this, [this](const QString& str) {
                        if ( this->m_slotDisabled )
                            return;

                        const auto actor = this->findActiveActor();
                        if ( nullptr != actor ) {
                            actor->m_name = str.toStdString();
                        }
                    }
                );
            }

            // Construct pos(3), quat(4), scale(1)
            {
                for ( unsigned i = 0; i < NUM_DOUBLE_VALUE; ++i ) {
                    constexpr QSizePolicy sizePolicy{ QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed };

                    auto& w = this->m_dspinboxes.construct(i, this);
                    this->m_grid.addWidget(&w, i + 1, 1, 1, 1);

                    w.setSizePolicy(sizePolicy);
                    w.setMinimumWidth(80);
                    w.setMaximumWidth(99999);

                    w.setMinimum(-DBL_MAX);
                    w.setMaximum(DBL_MAX);
                    w.setSingleStep(0.01);
                    w.setDecimals(3);
                    w.setSuffix(" m");
                }

                for ( unsigned i = 0; i < NUM_DOUBLE_VALUE; ++i ) {
                    auto& w = this->m_labels_forDspinboxes.construct(i, this);
                    this->m_grid.addWidget(&w, i + 1, 0, 1, 1, Qt::AlignRight);
                    w.setSizePolicy(this->LABEL_SIZE_POLICY);
                }
            }

            {
                this->m_labels_forDspinboxes[0].setText("Pos  x");
                this->m_labels_forDspinboxes[1].setText("y");
                this->m_labels_forDspinboxes[2].setText("z");

                this->m_labels_forDspinboxes[3].setText("Quat  w");
                this->m_labels_forDspinboxes[4].setText("x");
                this->m_labels_forDspinboxes[5].setText("y");
                this->m_labels_forDspinboxes[6].setText("z");

                this->m_labels_forDspinboxes[7].setText("Scale  xyz");
            }

            // Connect
            {
                const auto signalFunc = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

                QMetaObject::Connection _;

                auto posUpdateFunc = [this](const double v) {
                    if ( this->m_slotDisabled )
                        return;

                    auto actor = this->findActiveActor();
                    if ( nullptr != actor ) {
                        actor->m_trans.setPos(
                            this->m_dspinboxes[0].value(),
                            this->m_dspinboxes[1].value(),
                            this->m_dspinboxes[2].value()
                        );
                        this->m_shared.m_needRedraw = true;
                    }
                };

                _ = connect(&this->m_dspinboxes[0], signalFunc, this, posUpdateFunc);
                _ = connect(&this->m_dspinboxes[1], signalFunc, this, posUpdateFunc);
                _ = connect(&this->m_dspinboxes[2], signalFunc, this, posUpdateFunc);

                auto quatUpdateFunc = [this](const double v) {
                    if ( this->m_slotDisabled )
                        return;

                    auto actor = this->findActiveActor();
                    if ( nullptr != actor ) {
                        actor->m_trans.setQuat(
                            this->m_dspinboxes[4].value(),
                            this->m_dspinboxes[5].value(),
                            this->m_dspinboxes[6].value(),
                            this->m_dspinboxes[3].value()
                        );
                        this->m_shared.m_needRedraw = true;
                    }
                };

                _ = connect(&this->m_dspinboxes[3], signalFunc, this, quatUpdateFunc);
                _ = connect(&this->m_dspinboxes[4], signalFunc, this, quatUpdateFunc);
                _ = connect(&this->m_dspinboxes[5], signalFunc, this, quatUpdateFunc);
                _ = connect(&this->m_dspinboxes[6], signalFunc, this, quatUpdateFunc);

                _ = connect(
                    &this->m_dspinboxes[7], signalFunc,
                    this, [this](const double v) {
                        if ( this->m_slotDisabled )
                            return;

                        auto actor = this->findActiveActor();
                        if ( nullptr != actor ) {
                            actor->m_trans.setScale(v);
                            this->m_shared.m_needRedraw = true;
                        }
                    }
                );
            }
        }

        virtual void onSharedInfoUpdated(void) override {
            auto ptr = this->findActiveActor();
            if ( nullptr == ptr ) {
                this->disable();
                return;
            }

            this->enable();
            auto& actor = *ptr;

            this->m_slotDisabled = true;
            {
                this->m_lineEdit_name.setText(actor.m_name.c_str());

                this->m_dspinboxes[0].setValue(actor.m_trans.pos().x);
                this->m_dspinboxes[1].setValue(actor.m_trans.pos().y);
                this->m_dspinboxes[2].setValue(actor.m_trans.pos().z);

                this->m_dspinboxes[3].setValue(actor.m_trans.quat().w);
                this->m_dspinboxes[4].setValue(actor.m_trans.quat().x);
                this->m_dspinboxes[5].setValue(actor.m_trans.quat().y);
                this->m_dspinboxes[6].setValue(actor.m_trans.quat().z);

                this->m_dspinboxes[7].setValue(actor.m_trans.scale());
            }
            this->m_slotDisabled = false;
        }

    private:
        void enable(void) {
            this->m_lineEdit_name.setDisabled(false);

            for ( unsigned i = 0; i < NUM_DOUBLE_VALUE; ++i ) {
                auto& w = this->m_dspinboxes[i];
                w.setDisabled(false);
            }
        }
        void disable(void) {
            this->m_lineEdit_name.setDisabled(true);

            for ( unsigned i = 0; i < NUM_DOUBLE_VALUE; ++i ) {
                auto& w = this->m_dspinboxes[i];
                w.setDisabled(true);
            }
        }

        dal::Actor* findActiveActor(void) const {
            if ( !this->m_shared.m_active ) {
                return nullptr;
            }

            const auto id = *this->m_shared.m_active;
            if ( !this->m_scene.m_entt.has<dal::Actor>(id) ) {
                return nullptr;
            }

            auto& actor = this->m_scene.m_entt.get<dal::Actor>(id);
            return &actor;
        }

    };


    class DataView_Useless : public PropertyPage {

    private:
        static constexpr unsigned NUM = 20;
        dal::NoInitArray<QLineEdit, NUM> m_lineedits;;

    public:
        DataView_Useless(QWidget* const parent, dal::Scene& scene, dal::SharedInfo& shared)
            : PropertyPage(parent, scene, shared)
        {
            for ( unsigned i = 0; i < this->NUM; ++i ) {
                auto& w = this->m_lineedits.construct(i, this);
                this->m_grid.addWidget(&w);
            }
        }

    };

}


namespace dal {

    class SelectableWidget : public QWidget {

    private:
        QGridLayout m_grid;

        DataView_Actor m_view_actor;
        DataView_Useless m_view_useless;

        PropertyPage* m_ptrs[2] = {
            &m_view_actor,
            &m_view_useless
        };

    public:
        SelectableWidget(QWidget* const parent, Scene& scene, SharedInfo& shared)
            : QWidget(parent)
            , m_view_actor(this, scene, shared)
            , m_view_useless(this, scene, shared)
        {
            this->setLayout(&this->m_grid);

            this->m_grid.addWidget(&this->m_view_actor);
            this->m_grid.addWidget(&this->m_view_useless);
        }

        void selectView(const unsigned index) {
            for ( unsigned i = 0; i < 2; ++i ) {
                if ( index == i ) {
                    this->m_ptrs[i]->show();
                }
                else {
                    this->m_ptrs[i]->hide();
                }
            }
        }

        void onSharedInfoUpdated(void) {
            m_view_actor.onSharedInfoUpdated();
            m_view_useless.onSharedInfoUpdated();
        }

    };


    class PropertyView::Impl {

    public:
        Scene& m_scene;
        SharedInfo& m_shared;

        QGridLayout m_grid;
        QComboBox m_viewSelector;
        QScrollArea m_scrollArea;
        SelectableWidget m_views;

    public:
        Impl(QWidget* const parent, Scene& scene, SharedInfo& shared)
            : m_scene(scene)
            , m_shared(shared)
            , m_grid(parent)
            , m_viewSelector(parent)
            , m_scrollArea(parent)
            , m_views(parent, scene, shared)
        {

        }

    };


    PropertyView::PropertyView(QWidget* const parent, Scene& scene, SharedInfo& shared)
        : QWidget(parent)
        , m_pimpl(new Impl{ this, scene, shared })
    {
        {
            auto& w = this->m_pimpl->m_viewSelector;
            w.addItem("Actor");
            w.addItem("Useless");

            auto _ = connect(
                &w, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, [this](const int v) {
                    this->m_pimpl->m_views.selectView(v);
                }
            );
        }

        {
            this->setLayout(&this->m_pimpl->m_grid);
            this->m_pimpl->m_grid.addWidget(&this->m_pimpl->m_viewSelector);
            this->m_pimpl->m_grid.addWidget(&this->m_pimpl->m_scrollArea);

            this->m_pimpl->m_scrollArea.setWidgetResizable(true);
            this->m_pimpl->m_scrollArea.setAlignment(Qt::AlignTop);
            this->m_pimpl->m_scrollArea.setWidget(&this->m_pimpl->m_views);
        }

        this->onSharedInfoUpdated();
    }

    PropertyView::~PropertyView(void) {
        delete this->m_pimpl;
        this->m_pimpl = nullptr;
    }

    void PropertyView::onSharedInfoUpdated(void) {
        this->m_pimpl->m_views.onSharedInfoUpdated();
    }

}
