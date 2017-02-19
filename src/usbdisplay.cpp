#include "usbdisplay.h"
#include "ui_usbdisplay.h"

static QMap<QString, QString> *devices;

USBDisplay::USBDisplay(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::USBDisplay)
{
    ui->setupUi(this);
    this->populate();
}

USBDisplay::~USBDisplay()
{
    delete devices;
    delete ui;
}

void USBDisplay::populate()
{
    devices = USB_device_listing();

    QList<QString> keys = devices->keys();

    foreach (QString key, keys) {
        QString size = device_size_string(devices->value(key));
        ui->deviceList->addItem(QString("%1 %2").arg(size.leftJustified(7, ' '), key));
    }
}

void USBDisplay::on_refreshButton_clicked()
{
    ui->deviceList->clear();
    delete devices;
    this->populate();
}

void USBDisplay::on_mountButton_clicked()
{
    QListWidgetItem *current = ui->deviceList->currentItem();

    if (current) {
        QStringList tokens = current->text().split(' ');
        tokens.removeFirst();

        QString name = tokens.join(' ').trimmed();
        QString device = devices->value(name);
        int r = mount_device(device);

        if (r == -1) {
            QMessageBox::critical(this, "Device Manager", errno_to_message());
            return;
        }

        if (CONFile::verifyDirectoryStructure(mounted_path())) {
            emit mounted_signal(name);
            this->hide();
        } else {
            unmount_device();
            QMessageBox::critical(this, "Device Manager", "Directory path not standard for an Xbox 360 drive.");
        }
    }
}
