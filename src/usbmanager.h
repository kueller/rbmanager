#ifndef USBMANAGER_H
#define USBMANAGER_H

#include <QDir>
#include <QMap>
#include <QProcess>

QMap<QString, QString> *USB_device_listing();
QString mounted_path();
void set_mounted_path(QString path);
bool device_mounted();
QString device_size_string(QString device);
int mount_device(QString device);
int unmount_device();
QString errno_to_message();

#endif // USBMANAGER_H
