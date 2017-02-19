#ifndef USBDISPLAY_H
#define USBDISPLAY_H

#include <QDialog>
#include <usbmanager.h>

#include "rbmanager.h"
#include "confile.h"

namespace Ui {
class USBDisplay;
}

class USBDisplay : public QDialog
{
    Q_OBJECT

public:
    explicit USBDisplay(QWidget *parent = 0);
    ~USBDisplay();

    void populate();

private slots:
    void on_refreshButton_clicked();

    void on_mountButton_clicked();

private:
    Ui::USBDisplay *ui;

signals:
    void mounted_signal(QString drive_name);
};

#endif // USBDISPLAY_H
