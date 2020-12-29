#include "copier.h"
#include <QThread>
#include <QEventLoop>
#include <QDebug>

Copier::Copier(QList<CONFile *> *files, QString save_path)
{
    this->files = files;
    this->save_path = save_path;
    this->added = 0;
}

void Copier::addSingleFile(CONFile *c, bool overwrite, bool save, int overwrite_index, QString overwrite_path)
{

    /*
    qDebug() << "filename";
    qDebug() << c->local_filepath;
    qDebug() << "overwrite path";
    qDebug() << overwrite_path;
    */

    if (save && !overwrite) {
        if (c->writeFile(save_path)) {
            files->append(c);
            items.append(c->item);

            added++;
        }
    } else if (save && overwrite) {
        if (c->overwriteFile(save_path, overwrite_path)) {

            /*
            qDebug() << "to delete";
            qDebug() << files->at(overwrite_index)->local_filepath;
            */
            delete files->at(overwrite_index);
            files->removeAt(overwrite_index);
            files->insert(overwrite_index, c);

            items.append(c->item);

            added++;
        }
    } else if (!save && !overwrite) {
        delete c;
    }

    emit updateAddProgress(++progress);
    if (progress >= total) {
        QThread::msleep(1000);
        emit finishAdd(items, added, total);
    }
}

void Copier::addFiles(QStringList filenames)
{
    QList<QTreeWidgetItem *> items;

    total = filenames.length();
    progress = 0;
    //qDebug() << "length: " << filenames.length();

    foreach (QString file, filenames) {
        if (CONFile::isCONFile(file)) {
            CONFile *c = new CONFile(file);

            //qDebug() << "Checking file: " << file;
            emit checkFile(filenames.length(), c);
        } else {
            progress++;
        }
    }
}
