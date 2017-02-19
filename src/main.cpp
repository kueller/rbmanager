#include "rbmanager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    RBManager w;
    w.show();

    return a.exec();
}
