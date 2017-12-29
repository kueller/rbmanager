#include <QTimer>

#include "rbmanager.h"
#include "ui_rbmanager.h"

USBDisplay *devices;
QList<CONFile *> *files;

QString current_user_directory = "/";
QString current_search_directory = "/";

QString status_text = "";

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

    devices = new USBDisplay(this);

    connect(devices, SIGNAL(mounted_signal(QString)), this, SLOT(set_mounted(QString)));

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

    QList<QTreeWidgetItem *> items;

    bool yes_to_all = false;
    bool no_to_all = false;

    int added = 0;

    foreach (QString file, filenames) {
        if (CONFile::isCONFile(file)) {
            CONFile *c = new CONFile(file);

            bool save = true;
            bool overwrite = false;

            int overwrite_index = 0;
            QString overwrite_path;

            for (int i = 0; i < files->size(); i++) {
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

                        if (filenames.length() > 1) {
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

            if (save && !overwrite) {
                if (c->writeFile(save_path)) {
                    files->append(c);
                    items.append(c->item);

                    added++;
                }
            } else if (save && overwrite) {
                if (c->overwriteFile(save_path, overwrite_path)) {

                    delete files->at(overwrite_index);
                    files->removeAt(overwrite_index);
                    files->append(c);

                    items.append(c->item);

                    added++;
                }
            } else if (!save && !overwrite) {
                delete c;
            }
        }
    }

    ui->fileList->addTopLevelItems(items);
    ui->fileList->sortItems(ui->fileList->sortColumn(), Qt::AscendingOrder);

    ui->statusLabel->setText(QString("Added %1 of %2 files.").arg(added).arg(filenames.length()));
    QTimer::singleShot(4000, this, SLOT(reset_status()));
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

void RBManager::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
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
