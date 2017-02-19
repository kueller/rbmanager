#ifndef RBMANAGER_H
#define RBMANAGER_H

#include <QMainWindow>
#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>

#include <usbdisplay.h>

#include "usbmanager.h"
#include "confile.h"

namespace Ui {
class RBManager;
}

class RBManager : public QMainWindow
{
    Q_OBJECT

public:
    explicit RBManager(QWidget *parent = 0);
    ~RBManager();
    void reset_list();
    void populate_list(QString path);
    void RB_unmount();

public slots:
    void set_mounted(QString drive_name);

private slots:
    void keyPressEvent(QKeyEvent *event);

    void on_openUSBManager_clicked();

    void on_actionQuit_triggered();

    void on_actionDevice_Manager_triggered();

    void on_actionUnmount_Drive_triggered();

    void on_actionOpen_File_triggered();

    void on_actionPoint_to_local_directory_triggered();

private:
    Ui::RBManager *ui;
};



#endif // RBMANAGER_H
