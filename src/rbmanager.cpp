#include <QTimer>
#include <QThread>

#include "rbmanager.h"
#include "ui_rbmanager.h"

USBDisplay *devices;
CONFileInfo *coninfo;
Copier *copier;

QList<CONFile *> *files;

QString current_user_directory = "/";
QString current_search_directory = "/";

QString status_text = "";

bool yes_to_all = false;
bool no_to_all = false;

RBManager::RBManager(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RBManager)
{
    ui->setupUi(this);
    ui->fileList->setAcceptDrops(true);

    setAcceptDrops(true);

    ui->actionDevice_Manager->setEnabled(true);
    ui->actionUnmount_Drive->setEnabled(false);
    ui->actionOpen_File->setEnabled(false);
    ui->progressBar->setVisible(false);
    ui->progressBar->setFormat("%v/%m");

    devices = new USBDisplay(this);
    coninfo = new CONFileInfo(this);

    connect(devices, SIGNAL(mounted_signal(QString)), this, SLOT(set_mounted(QString)));

    QHeaderView *headers = ui->fileList->header();
    headers->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(headers, &QTreeWidget::customContextMenuRequested, this, &RBManager::on_headers_customContextMenuRequested);

    files = new QList<CONFile *>;
}


RBManager::~RBManager()
{
    foreach (CONFile *c, *files) {
        delete c;
    }

    delete files;

    if (device_mounted()) {
        unmount_device();
    }

    delete ui;
}

void RBManager::reset_list()
{
    ui->fileList->setColumnCount(0);
    foreach (CONFile *c, *files) {
        delete c;
    }

    ui->fileList->clear();
    files->clear();
}

void RBManager::populate_list(QString path)
{
    ui->fileList->setColumnCount(3);
    ui->fileList->setHeaderLabels(QStringList() << "Artist" << "Name" << "Album");

    QDir dir(CONFile::songsDirectory(path));

    QList<QTreeWidgetItem *> items;

    foreach (QString file, dir.entryList()) {
        if (CONFile::isCONFile(dir.absoluteFilePath(file))) {
            CONFile *c = new CONFile(dir.absoluteFilePath(file));
            files->append(c);
            items.append(c->item);
        }
    }

    ui->fileList->addTopLevelItems(items);
    ui->fileList->sortItems(0, Qt::AscendingOrder);
    ui->fileList->header()->resizeSections(QHeaderView::Stretch);
}

void RBManager::set_mounted(QString drive_name)
{
    ui->openUSBManager->setText("Unmount");
    ui->actionDevice_Manager->setEnabled(false);
    ui->actionUnmount_Drive->setEnabled(true);
    ui->actionPoint_to_local_directory->setEnabled(false);
    ui->actionOpen_File->setEnabled(true);
    ui->statusLabel->setText(QString("Drive: \"%1\"").arg(drive_name));
    ui->searchBox->setEnabled(true);

    this->reset_list();
    this->populate_list(mounted_path());

    status_text = ui->statusLabel->text();
}

void RBManager::reset_status()
{
    ui->statusLabel->setText(status_text);
}

void RBManager::RB_unmount()
{
    int r = unmount_device();
    if (r != 0) {
        QMessageBox::critical(this, "RBManager", errno_to_message());
    }

    ui->openUSBManager->setText("Open Drive");

    ui->actionUnmount_Drive->setEnabled(false);
    ui->actionDevice_Manager->setEnabled(true);
    ui->actionPoint_to_local_directory->setEnabled(true);
    ui->actionOpen_File->setEnabled(false);
    ui->statusLabel->setText("");
    ui->searchBox->setText("");
    ui->searchBox->setEnabled(false);

    this->reset_list();
    status_text = "";
}

void RBManager::addFiles(QStringList filenames)
{
    QString save_path;
    if (device_mounted()) {
        save_path = mounted_path();
    } else {
        save_path = current_user_directory;
    }

    yes_to_all = false;
    no_to_all = false;

    ui->progressBar->reset();
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(filenames.length());
    ui->progressBar->setVisible(true);

    thread = new QThread();

    copier = new Copier(files, save_path);

    connect(copier, SIGNAL(updateAddProgress(int)), this, SLOT(update_progress(int)));
    connect(copier, SIGNAL(checkFile(int, CONFile *)), this, SLOT(checkNewFileExists(int, CONFile *)), Qt::BlockingQueuedConnection);
    connect(copier, SIGNAL(finishAdd(QList<QTreeWidgetItem *>, int, int)), this, SLOT(finish_add(QList<QTreeWidgetItem *>, int, int)));

    connect(this, SIGNAL(startAdd(QStringList)), copier, SLOT(addFiles(QStringList)), Qt::QueuedConnection);
    connect(this, SIGNAL(finishAddCheck(CONFile *, bool, bool, int, QString)), copier, SLOT(addSingleFile(CONFile *, bool, bool, int, QString)));

    copier->moveToThread(thread);

    emit startAdd(filenames);
    thread->start();
}

void RBManager::filterItems(QString query)
{
    for (int i = 0; i < ui->fileList->topLevelItemCount(); i++) {
        QTreeWidgetItem *t = ui->fileList->topLevelItem(i);

        bool matched = false;

        matched = t->text(0).contains(query, Qt::CaseInsensitive);
        matched = matched || t->text(1).contains(query, Qt::CaseInsensitive);
        matched = matched || t->text(2).contains(query, Qt::CaseInsensitive);

        t->setHidden(!matched);
    }
}

void RBManager::update_progress(int value)
{
    ui->progressBar->setValue(value);
}

void RBManager::finish_add(QList<QTreeWidgetItem *> items, int added, int total)
{
    ui->fileList->addTopLevelItems(items);
    ui->fileList->sortItems(ui->fileList->sortColumn(), Qt::AscendingOrder);

    disconnect(copier, SIGNAL(updateAddProgress(int)), this, SLOT(update_progress(int)));
    disconnect(copier, SIGNAL(checkFile(int, CONFile *)), this, SLOT(checkNewFileExists(int, CONFile *)));
    disconnect(copier, SIGNAL(finishAdd(QList<QTreeWidgetItem *>, int, int)), this, SLOT(finish_add(QList<QTreeWidgetItem *>, int, int)));

    disconnect(this, SIGNAL(startAdd(QStringList)), copier, SLOT(addFiles(QStringList)));
    disconnect(this, SIGNAL(finishAddCheck(CONFile *, bool, bool, int, QString)), copier, SLOT(addSingleFile(CONFile *, bool, bool, int, QString)));

    free(copier);

    ui->statusLabel->setText(QString("Added %1 of %2 files.").arg(added).arg(total));
    QTimer::singleShot(5000, this, SLOT(reset_status()));
    ui->progressBar->setVisible(false);
}

void RBManager::checkNewFileExists(int length, CONFile *c)
{
    bool save = true;
    bool overwrite = false;

    int overwrite_index = 0;
    int files_len = files->size();

    QString overwrite_path;

    for (int i = 0; i < files_len; i++) {
        if (files->at(i)->filename == c->filename) {
            if (no_to_all) {
                save = false;
                break;
            }

            if (!yes_to_all && !no_to_all) {
                QMessageBox alert;
                alert.setWindowTitle("RBManager");
                alert.setText(QString("Custom \"%1\" exists in directory.").arg(c->filename));
                alert.setInformativeText("Overwrite existing file?");

                if (length > 1) {
                    alert.setStandardButtons(QMessageBox::Yes | QMessageBox::YesToAll|
                                 QMessageBox::No | QMessageBox::NoToAll);
                } else {
                    alert.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                }

                alert.setDefaultButton(QMessageBox::No);

                int ret = alert.exec();

                yes_to_all = ret == QMessageBox::YesToAll;
                no_to_all = ret == QMessageBox::NoToAll;
                save = (ret == QMessageBox::Yes) | yes_to_all;
            } else {
                save = true;
            }

            overwrite = save;
            overwrite_path = files->at(i)->local_filepath;
            overwrite_index = i;

            break;
        }
    }

    emit finishAddCheck(c, overwrite, save, overwrite_index, overwrite_path);
}

void RBManager::deleteFiles()
{
    QList<QTreeWidgetItem *> selected;
    selected = ui->fileList->selectedItems();

    if (selected.size() == 0) return;

    QString q;
    if (selected.size() == 1) {
        q = "Delete 1 selected file?";
    } else {
        q = QString("Delete %1 selected files?").arg(selected.size());
    }

    QMessageBox alert;
    alert.setText(q);
    alert.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    alert.setDefaultButton(QMessageBox::Ok);

    int ret = alert.exec();

    if (ret == QMessageBox::Ok) {
        QList<CONFile *> todelete;
        for (int i = 0; i < files->size(); i++) {
            foreach (QTreeWidgetItem *item, selected) {
                if (item == files->at(i)->item) {
                    todelete.append(files->at(i));
                    break;
                }
            }
        }

        foreach (CONFile *c, todelete) {
            QFile f(c->local_filepath);
            f.remove();
            files->removeOne(c);
            delete c;
        }
    }
}

void RBManager::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        deleteFiles();
    }
}

void RBManager::dropEvent(QDropEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        QList<QUrl> urls = e->mimeData()->urls();
        QStringList files;

        for (int i = 0; i < urls.size(); i++) {
            files.append(urls.at(i).toLocalFile());
        }

        if (files.size() > 0) {
            this->addFiles(files);
        }
    }
}

void RBManager::dragEnterEvent(QDragEnterEvent *e)
{
    e->accept();
}

void RBManager::on_openUSBManager_clicked()
{
    if (!device_mounted()) {
        devices->show();
    } else {
        this->RB_unmount();
    }
}

void RBManager::on_actionCONFile_Info_triggered()
{
    if (!coninfo->isVisible()) {
        coninfo->show();
    }
}

void RBManager::on_actionQuit_triggered()
{
    this->~RBManager();
}

void RBManager::on_actionDevice_Manager_triggered()
{
    if (!device_mounted()) {
        devices->show();
    }
}

void RBManager::on_actionUnmount_Drive_triggered()
{
    if (device_mounted()) {
        this->RB_unmount();
    }
}

void RBManager::on_actionOpen_File_triggered()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setDirectory(current_search_directory);
    dialog.setAcceptDrops(true);

    QStringList filenames;
    if (dialog.exec()) {
        filenames = dialog.selectedFiles();
        QFileInfo info(filenames.at(0));
        current_search_directory = info.canonicalPath();

        this->addFiles(filenames);
    }
}

void RBManager::on_actionPoint_to_local_directory_triggered()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle("RBManager");
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setDirectory(current_user_directory);

    QStringList filenames;
    if (dialog.exec()) {
        filenames = dialog.selectedFiles();
        QString path = filenames[0];
        this->reset_list();
        this->populate_list(path);
        ui->actionOpen_File->setEnabled(true);
        ui->searchBox->setText("");
        ui->searchBox->setEnabled(true);

        current_user_directory = path;
        ui->statusLabel->setText(QString("Directory: \"%1\"").arg(path));
        status_text = ui->statusLabel->text();
    }
}

void RBManager::on_searchBox_textEdited(const QString &arg1)
{
    this->filterItems(arg1);
}

void RBManager::on_fileList_customContextMenuRequested(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(ui->actionDelete);
    menu.addAction(ui->actionInfo);

    ui->actionDelete->setData(QVariant(pos));
    ui->actionInfo->setData(QVariant(pos));

    menu.exec(ui->fileList->mapToGlobal(pos));
}

void RBManager::on_actionDelete_triggered()
{
    deleteFiles();
}

void RBManager::on_actionInfo_triggered()
{
    QTreeWidgetItem *item = ui->fileList->itemAt(ui->actionInfo->data().toPoint());

    for (int i = 0; i < files->size(); i++) {
        if (item == files->at(i)->item) {
            coninfo->loadDrop(files->at(i)->local_filepath);
            files->at(i)->author();
            if (!coninfo->isVisible()) {
                coninfo->show();
            }
            break;
        }
    }
}

void RBManager::on_headers_customContextMenuRequested(QPoint pos)
{
    QMenu menu(this);
    menu.addAction(ui->actionSetHeaderArtist);
    menu.addAction(ui->actionSetHeaderSongName);
    menu.addAction(ui->actionSetHeaderAlbum);
    menu.addAction(ui->actionSetHeaderFilename);
    menu.addAction(ui->actionSetHeaderAuthor);

    ui->actionSetHeaderArtist->setData(QVariant(pos));
    ui->actionSetHeaderSongName->setData(QVariant(pos));
    ui->actionSetHeaderAlbum->setData(QVariant(pos));
    ui->actionSetHeaderFilename->setData(QVariant(pos));
    ui->actionSetHeaderAuthor->setData(QVariant(pos));

    QActionGroup hdrgrp(this);
    hdrgrp.addAction(ui->actionSetHeaderArtist);
    hdrgrp.addAction(ui->actionSetHeaderSongName);
    hdrgrp.addAction(ui->actionSetHeaderAlbum);
    hdrgrp.addAction(ui->actionSetHeaderFilename);
    hdrgrp.addAction(ui->actionSetHeaderAuthor);
    hdrgrp.setExclusive(true);

    int hdridx = ui->fileList->header()->logicalIndexAt(pos);
    QString htext = ui->fileList->headerItem()->text(hdridx);
    if (htext == "Artist") {
        ui->actionSetHeaderArtist->setChecked(true);
    } else if (htext == "Name") {
        ui->actionSetHeaderSongName->setChecked(true);
    } else if (htext == "Album") {
        ui->actionSetHeaderAlbum->setChecked(true);
    } else if (htext == "Filename") {
        ui->actionSetHeaderFilename->setChecked(true);
    } else if (htext == "Author") {
        ui->actionSetHeaderAuthor->setChecked(true);
    }

    menu.exec(ui->fileList->header()->mapToGlobal(pos));
}

void RBManager::update_column(int col, QString title)
{
    ui->fileList->headerItem()->setText(col, title);

    if (title == "Artist") {
        for (int i = 0; i < files->size(); i++) {
            files->at(i)->item->setText(col, files->at(i)->artist());
        }
    } else if (title == "Name") {
        for (int i = 0; i < files->size(); i++) {
            files->at(i)->item->setText(col, files->at(i)->songName());
        }
    } else if (title == "Album") {
        for (int i = 0; i < files->size(); i++) {
            files->at(i)->item->setText(col, files->at(i)->album());
        }
    } else if (title == "Filename") {
        for (int i = 0; i < files->size(); i++) {
            files->at(i)->item->setText(col, files->at(i)->filename);
        }
    } else if (title == "Author") {
        for (int i = 0; i < files->size(); i++) {
            files->at(i)->item->setText(col, files->at(i)->author());
        }
    }
}

void RBManager::on_actionSetHeaderArtist_triggered()
{
    int hdridx = ui->fileList->header()->logicalIndexAt(ui->actionSetHeaderArtist->data().toPoint());
    if (ui->fileList->headerItem()->text(hdridx) != "Artist") {
        update_column(hdridx, "Artist");
    }
}

void RBManager::on_actionSetHeaderSongName_triggered()
{
    int hdridx = ui->fileList->header()->logicalIndexAt(ui->actionSetHeaderSongName->data().toPoint());
    if (ui->fileList->headerItem()->text(hdridx) != "Name") {
        update_column(hdridx, "Name");
    }
}

void RBManager::on_actionSetHeaderAlbum_triggered()
{
    int hdridx = ui->fileList->header()->logicalIndexAt(ui->actionSetHeaderAlbum->data().toPoint());
    if (ui->fileList->headerItem()->text(hdridx) != "Album") {
        update_column(hdridx, "Album");
    }
}

void RBManager::on_actionSetHeaderFilename_triggered()
{
    int hdridx = ui->fileList->header()->logicalIndexAt(ui->actionSetHeaderFilename->data().toPoint());
    if (ui->fileList->headerItem()->text(hdridx) != "Filename") {
        update_column(hdridx, "Filename");
    }
}

void RBManager::on_actionSetHeaderAuthor_triggered()
{
    int hdridx = ui->fileList->header()->logicalIndexAt(ui->actionSetHeaderAuthor->data().toPoint());
    if (ui->fileList->headerItem()->text(hdridx) != "Author") {
        update_column(hdridx, "Author");
    }
}
