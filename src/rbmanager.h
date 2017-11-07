#ifndef RBMANAGER_H
#define RBMANAGER_H

#include <QMainWindow>
#include <QUrl>
#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>

#include <QDebug>

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
    void reset_status();

    void addFiles(QStringList filenames);

    void filterItems(QString query);

    void keyPressEvent(QKeyEvent *event);

    void on_openUSBManager_clicked();

    void on_actionQuit_triggered();

    void on_actionDevice_Manager_triggered();

    void on_actionUnmount_Drive_triggered();

    void on_actionOpen_File_triggered();

    void on_actionPoint_to_local_directory_triggered();

    void on_searchBox_textEdited(const QString &arg1);

private:
    Ui::RBManager *ui;

protected:
    void dropEvent(QDropEvent *e);
    void dragEnterEvent(QDragEnterEvent *e);
};



#endif // RBMANAGER_H
