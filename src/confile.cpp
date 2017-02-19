#include "confile.h"
#include <QDebug>

const QString RB_CODE = "45410914";

CONData::CONData()
{
    this->list = new QList<CONData *>;
}

CONData::~CONData()
{
    if (!this->list->isEmpty()) {
        foreach (CONData *d, *(this->list)) {
            delete d;
        }
    }

    delete this->list;
}

CONFile::CONFile(QString filepath)
{
    this->local_filepath = filepath;
    this->raw_data = new QList<CONData *>;

    if (this->isCONFile(filepath)) {
        QString metadata = this->readMetadata(filepath);
        if (metadata == "") return;

        this->parseMetadata(metadata, this->raw_data, 0);
        this->filename = raw_data->at(0)->text;

        QStringList data;
        data.append(this->artist());
        data.append(this->songName());
        data.append(this->filename);

        this->item = new QTreeWidgetItem(data);
    }
}

CONFile::~CONFile()
{
    if (!this->raw_data->isEmpty()) {
        foreach (CONData *d, *(this->raw_data)) {
            delete d;
        }
    }

    delete this->item;
    delete this->raw_data;
}

QString CONFile::songName()
{
    CONData *d = this->raw_data->at(1);
    return d->list->at(1)->text;
}

QString CONFile::artist()
{
    CONData *d = this->raw_data->at(2);
    return d->list->at(1)->text;
}

bool CONFile::isCONFile(QString filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    char header[3];
    if (file.read(header, 3) == -1) {
        file.close();
        return false;
    }

    file.close();
    return QString::fromLocal8Bit(header, 3) == "CON";
}

QString CONFile::readMetadata(QString filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) return "";

    qint64 size = file.size();
    if (size < METADATA_START) return "";

    if (!file.seek(METADATA_START)) return "";

    QString metadata;
    char byte;

    do {
        file.read(&byte, 1);
        metadata.append(QChar(byte));
    } while (byte != 0);

    file.close();
    return metadata;
}

int CONFile::parseMetadata(QString raw_text, QList<CONData *> *list, int i)
{
    QString s = "";
    bool read_active = false;

    for (; raw_text[i] != '('; i++);

    i++;
    for (; raw_text[i] != ')'; i++) {
        if (raw_text[i] == '\'') {
            i++;
            for (; raw_text[i] != '\''; i++) {
                s.append(raw_text[i]);
            }

            CONData *c = new CONData();
            c->text = s;
            list->append(c);
            s = "";
        } else if (raw_text[i] == '\"') {
            i++;
            for (; raw_text[i] != '\"'; i++) {
                s.append(raw_text[i]);
            }
            CONData *c = new CONData();
            c->text = s;
            list->append(c);
            s = "";
        } else if (raw_text[i] == ';') {
            for (; raw_text[i] != '\n'; i++);
        } else if (raw_text[i] == '(') {
            CONData *c = new CONData();
            i = parseMetadata(raw_text, c->list, i);
            list->append(c);
        } else if (!raw_text[i].isSpace() && !read_active) {
            s.append(raw_text[i]);
            read_active = true;
        } else if (!raw_text[i].isSpace() && read_active) {
            s.append(raw_text[i]);
        } else if (raw_text[i].isSpace() && read_active) {
            CONData *c = new CONData();
            c->text = s;
            list->append(c);
            s = "";
            read_active = false;
        }
    }

    if (read_active) {
        CONData *c = new CONData();
        c->text = s;
        list->append(c);
    }

    return i;
}

void CONFile::writeFile(QString mount_path)
{
    QDir dir(mount_path);
    dir.cd(QString("Content/0000000000000000/%1/00000001").arg(RB_CODE));

    QString basename = this->filename.left(6) + "~";

    int counter = 1;
    for (; counter < 9; counter++) {
        if (!dir.exists(basename + QString::number(counter, 10)))
            break;
    }

    QString filename;

    if (dir.exists(basename + QString::number(counter, 10))) {
        // handle alphabet
    } else {
        filename = basename + QString::number(counter, 10);
    }

    QFile::copy(this->local_filepath, dir.absoluteFilePath(filename));
    this->local_filepath = dir.absoluteFilePath(filename);
}

void CONFile::overwriteFile(QString mount_path, QString existing_filepath)
{
    QDir dir(mount_path);
    dir.cd(QString("Content/0000000000000000/%1/00000001").arg(RB_CODE));

    if (dir.exists(existing_filepath)) {
        QFile::remove(existing_filepath);
    }

    QFile::copy(this->local_filepath, existing_filepath);
    this->local_filepath = existing_filepath;
}

bool CONFile::verifyDirectoryStructure(QString mount_path)
{
    QDir dir(mount_path);
    if (!dir.exists("Content")) return false;
    dir.cd("Content");

    if (!dir.exists("0000000000000000")) return false;
    dir.cd("0000000000000000");

    if (!dir.exists(RB_CODE)) {
        dir.mkdir(RB_CODE);
    }

    dir.cd(RB_CODE);

    if (!dir.exists("00000001"))
        dir.mkdir("00000001");
    if (!dir.exists("00000002"))
        dir.mkdir("00000002");
    if (!dir.exists("000B0000"))
        dir.mkdir("000B0000");

    dir.cd("00000001");
    return true;
}

QString CONFile::songsDirectory(QString mount_path)
{
    QDir dir(mount_path);
    dir.cd(QString("Content/0000000000000000/%1/00000001").arg(RB_CODE));
    return dir.absolutePath();
}


