#include <QtWidgets/QApplication>

#include "d_dalbaragimapper.h"


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    dal::DalbaragiMapper w;
    w.show();
    return a.exec();
}
