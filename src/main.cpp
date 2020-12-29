#include "rbmanager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QList<QTreeWidgetItem*>>("QList<QTreeWidgetItem*>");
    QApplication a(argc, argv);
    RBManager w;
    w.show();

    return a.exec();
}
