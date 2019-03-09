#ifndef CONFILEINFO_H
#define CONFILEINFO_H

#include <QDialog>
#include <QtGui>
#include <QUrl>
#include <QDropEvent>
#include <QPixmap>

#include "confile.h"

namespace Ui {
class CONFileInfo;
}

class CONFileInfo : public QDialog
{
    Q_OBJECT

public:
    explicit CONFileInfo(QWidget *parent = 0);
    ~CONFileInfo();

    void displayInfo(CONFile *c);
protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
private:
    Ui::CONFileInfo *ui;
    void loadDrop(QString filename);
    void resetWindow();
};

#endif // CONFILEINFO_H
