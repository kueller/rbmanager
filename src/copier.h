#ifndef COPIER_H
#define COPIER_H

#include <QString>
#include <QStringList>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QList>
#include <QtTest/QSignalSpy>
#include "confile.h"

/*
 * Structure of Copier:
 * 1. Main thread creates Copier when new customs are added and launches
 *    on a new thread.
 * 2. If only one file, main thread calls addSingleFile (go to step 5).
 * 3. If multiple, main thread calls addFiles in Copier thread. addFiles
 *    loops and calls checkFile for each CON to confirm overwrites (in main thread).
 * 4. Main thread, after checking, calls addSingleFile for each file.
 * 5. Copier thread copies files then calls finishAdd which disconnects signals
 *    and frees the Copier memory.
 */
class Copier :public QObject
{
    Q_OBJECT

public:
    explicit Copier(QList<CONFile *> *files, QString save_path);
private:
    QList<CONFile *> *files;
    QString save_path;
    QList<QTreeWidgetItem *> items;
    int added;
    int progress;
    int total;
public slots:
    void addFiles(QStringList filenames);
    void addSingleFile(CONFile *c, bool overwrite, bool save, int overwrite_index, QString overwrite_path);
signals:
    void updateAddProgress(int value);
    void checkFile(int length, CONFile *c);
    void finishAdd(QList<QTreeWidgetItem *> items, int added, int total);
};

#endif // COPIER_H
