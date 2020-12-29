#ifndef RBMANAGER_H
#define RBMANAGER_H

#include <QMainWindow>
#include <QUrl>
#include <QtGui>
#include <QMessageBox>
#include <QErrorMessage>
#include <QFileDialog>
#include <QProcess>

#include <QDebug>

#include <usbdisplay.h>

#include "usbmanager.h"
#include "confileinfo.h"
#include "confile.h"
#include "copier.h"

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
    void update_progress(int value);

    /*
     * Final slot. Disconnects signals and frees copier memory. Adds new
     * items to the main files list.
     */
    void finish_add(QList<QTreeWidgetItem *> items, int added, int total);

    /*
     * Checks if file exists to prompt dialogue box (yes/no/yes or not to all).
     * Called from the addFiles in Copier and calls the addSingleFile function
     * for each CONFile in the Copier. Must be called with a blocking signal
     * or it will launch multiple message boxes.
     */
    void checkNewFileExists(int length, CONFile *c);
private slots:
    void reset_status();

    /*
     * Create a new thread to launch the copier on, set up slots
     * reset all values and progress bars, and launch.
     * This will emit signal to addFiles in the Copier.
     */
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

    void on_actionCONFile_Info_triggered();
    void on_fileList_customContextMenuRequested(const QPoint &pos);

    void on_actionDelete_triggered();

    void on_actionInfo_triggered();

    void on_headers_customContextMenuRequested(QPoint pos);
    void on_actionSetHeaderArtist_triggered();

    void on_actionSetHeaderSongName_triggered();

    void on_actionSetHeaderAlbum_triggered();

    void on_actionSetHeaderFilename_triggered();

    void on_actionSetHeaderAuthor_triggered();

private:
    Ui::RBManager *ui;
    QThread *thread;

    void deleteFiles();
    void update_column(int col, QString title);
protected:
    void dropEvent(QDropEvent *e);
    void dragEnterEvent(QDragEnterEvent *e);

signals:
   void startAdd(QStringList filenames);
   void finishAddCheck(CONFile *c, bool overwrite, bool save, int overwrite_index, QString overwrite_path);

};



#endif // RBMANAGER_H
