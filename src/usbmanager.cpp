#include "usbmanager.h"
#include "size.h"

#include <time.h>
#include <errno.h>
#include <cstdint>

#include <sys/mount.h>

#define IS_NUMBER(x) ((x) >= 48 && (x) <= 57)

static QString mount_path;
static bool mounted = false;

QString parse_device_name(QString filename)
{
    QStringList sections = filename.split('-');
    QStringList product;

    foreach(QString token, sections[1].split('_')) {
        if (IS_NUMBER(token[0])) break;
        product.append(token);
    }

    return QString(product.join(' '));
}

QMap<QString, QString> *USB_device_listing()
{
    QDir dir("/dev/disk/by-id");
    QMap<QString, QString> *map = new QMap<QString, QString>;

    QStringList ls = dir.entryList();

    foreach (QString device, ls) {
        if (!device.split('-')[0].compare("usb")) {
            map->insert(parse_device_name(device), dir.absoluteFilePath(device));
        }
    }

    return map;
}

QString errno_to_message()
{
    char *err = strerror(errno);
    return QString::fromUtf8(err);
}

QString mounted_path()
{
    return mount_path;
}

void set_mounted_path(QString path)
{
    mount_path = path;
}

bool device_mounted()
{
    return mounted;
}

QString device_size_string(QString device)
{
    uint64_t size = device_file_size(device.toStdString().c_str());

    QString size_f;
    if (size <= 0) {
        size_f = "-";
    } else {
        if (size >= 1000000000) {
            size_f = QString("%1G").arg(QString::number((float)size / (1024 * 1024 * 1024), 'f', 1));
        } else {
            size_f = QString("%1M").arg(QString::number(size / (1024 * 1024)));
        }
    }

    return size_f;
}

int mount_device(QString device)
{
    mount_path = QString("/media/RB_%1").arg(QString::number(time(NULL)));

    if (!QDir(mount_path).exists()) {
        QDir().mkdir(mount_path);
    }

    int r = mount(device.toStdString().c_str(), mount_path.toStdString().c_str(), "vfat", MS_NOATIME, NULL);

    if (r != 0) {
        QDir().rmdir(mount_path);
    }

    mounted = r == 0;
    return r;
}

int unmount_device()
{
    if (!mounted) return -1;
    int r = umount2(mount_path.toStdString().c_str(), 0);

    if (r == 0) {
        QDir().rmdir(mount_path);
    }

    mounted = r != 0;
    return r;
}
