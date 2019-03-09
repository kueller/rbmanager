#include "confileinfo.h"
#include "ui_confileinfo.h"

#define TITLE_MAX 22
#define ARTIST_MAX 28

static CONFile *con = NULL;

CONFileInfo::CONFileInfo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CONFileInfo)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    this->resetWindow();
}

CONFileInfo::~CONFileInfo()
{
    if (con) delete con;
    delete ui;
}

void CONFileInfo::resetWindow()
{
    ui->titleLabel->setText("");
    ui->artistLabel->setText("");
    ui->textEdit->setText("");

    con = NULL;
}

void CONFileInfo::displayInfo(CONFile *c)
{
    if (c->thumbnail.length() > 0) {
        QPixmap image;
        image.loadFromData(c->thumbnail, "PNG");
        ui->icon->setPixmap(image);
    }

    QString main_t = c->songName().left(TITLE_MAX);
    QString under_t = QString("%1, %2").arg(c->artist().left(ARTIST_MAX - 6)).arg(c->year());
    ui->titleLabel->setText(main_t);
    ui->artistLabel->setText(under_t);

    ui->textEdit->setText(c->dta);
}

void CONFileInfo::loadDrop(QString filename)
{
    if (CONFile::isCONFile(filename)) {
        if (con) delete con;
        con = new CONFile(filename);

        displayInfo(con);
    }
}

void CONFileInfo::dropEvent(QDropEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        QList<QUrl> urls = e->mimeData()->urls();
        QStringList files;

        if (urls.size() != 1) return;

        QString file = urls.at(0).toLocalFile();
        this->loadDrop(file);
    }
}

void CONFileInfo::dragEnterEvent(QDragEnterEvent *e)
{
    e->accept();
}
