#include "confile.h"
#include <cstdio>
#include <QDebug>
#include <QString>
#include <unistd.h>
#include <iostream>

const QString RB_CODE = "45410914";
const QString letters = "abcdefghijklmnopqrstuvwxyz";

const QByteArray IHDR = QByteArray("\x89\x50\x4e\x47\x0d\x0a\x1a\x0a");

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
        dta = this->readMetadata(filepath);

        if (dta == "") return;

        this->parseMetadata(dta, this->raw_data, 0);
        this->thumbnail = this->readThumbnail(filepath);
        this->filename = raw_data->at(0)->text;

        QStringList data;
        data.append(this->artist());
        data.append(this->songName());
        data.append(this->album());
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

QString CONFile::album()
{
    QString album;

    foreach (CONData *d, *(this->raw_data)) {
        if (d->list->length() == 0) continue;

        if (d->list->at(0)->text == "album_name") {
            album = d->list->at(1)->text;
            break;
        }
    }

    return album;
}

QString CONFile::year()
{
    QString year;

    foreach (CONData *d, *(this->raw_data)) {
        if (d->list->length() == 0) continue;

        if (d->list->at(0)->text == "year_released") {
            year = d->list->at(1)->text;
            break;
        }
    }

    return year;
}

QString CONFile::author()
{
    QString author;

    foreach (CONData *d, *(this->raw_data)) {
        if (d->text.startsWith(";Song authored by")) {
            author = d->text;
            author.remove(";Song authored by ");
        }
    }

    return author;
}

QString CONFile::genericFind(QString query)
{
    query = "WIP";
    return "WIP";
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

quint32 CONFile::blockToOffset(quint32 block_start, quint32 entry_id)
{
    quint32 block_adjust = 0;
    quint8 table_size_shift = 0;

    table_size_shift = (((entry_id + 0xfff) & 0xf000) >> 0xc) == 0xb;

    if (block_start >= 0xaa)
        block_adjust += ((block_start / 0xaa) + 1) << table_size_shift;
    if (block_start > 0x70e4)
        block_adjust += ((block_start / 0x70e4) + 1) << table_size_shift;

    return ((block_start + block_adjust) * BLOCK_SIZE) + BLOCK_START;
}

quint32 CONFile::getFileTypeOffset(QString filepath, QString extension)
{
    quint32 entry_id = 0;
    quint8 blockno[3];
    quint32 block_start = 0;
    quint32 file_list_offset = 0;

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) return 0;
    QDataStream in(&file);

    // Entry ID used to find the table shift in blockToOffset
    file.seek(ENTRY_ID);
    in.setByteOrder(QDataStream::LittleEndian);
    in >> entry_id;
    in.setByteOrder(QDataStream::BigEndian);

    // Finding the block location of the file list table.
    // Messy operations like these to deal with 24 bit values.
    file.seek(FILE_TABLE_BLOCK);
    in >> blockno[2] >> blockno[1] >> blockno[0];
    block_start = blockno[0];
    block_start |= blockno[1] << 8;
    block_start |= blockno[2] << (8 * 2);

    file_list_offset = blockToOffset(block_start, entry_id);
    file.seek(file_list_offset);

    char *name = new char[40];
    QString qname;
    quint32 file_start_block = 0;
    quint32 file_start_offset = 0;
    quint8 fblock[3];

    in.readRawData(name, CON_FILENAME_LEN);
    qname = QString::fromLocal8Bit(name, strlen(name));

    while (name[0]) {
        if (qname.endsWith(extension)) {
            file.seek(file.pos() + 7);
            in >> fblock[0] >> fblock[1] >> fblock[2];
            file_start_block = fblock[0];
            file_start_block |= fblock[1] << 8;
            file_start_block |= fblock[2] << (8 * 2);

            file_start_offset = blockToOffset(file_start_block, entry_id);

            file.seek(file.pos() + 14);
        } else {
            file.seek(file.pos() + 24);
        }

        in.readRawData(name, CON_FILENAME_LEN);
        qname = QString::fromLocal8Bit(name, strlen(name));
    }

    file.close();
    return file_start_offset;
}

QString CONFile::readMetadata(QString filepath)
{
    QFile file(filepath);
    quint32 dta_offset = getFileTypeOffset(filepath, "dta");
    if (dta_offset == 0) return "";

    if (!file.open(QIODevice::ReadOnly)) return "";
    file.seek(dta_offset);

    QString metadata;
    char byte;
    bool start = false;

    do {
        file.read(&byte, 1);

        // Don't start reading until the first paren.
        // This will break otherwise with the UTF-8 reader.
        if (!start) {
            start = byte == '(';
        }

        if (start) {
            // UTF-8 parsing
            if (byte & 0x80) {
                QByteArray b;
                b.append(byte);
                file.read(&byte, 1);
                b.append(byte);
                metadata.append(QString::fromUtf8(b));
            } else {
                metadata.append(QChar(byte));
            }
        }
    } while (byte != 0);

    file.close();
    return metadata;
}

QByteArray CONFile::readThumbnail(QString filepath)
{
    bool end = false;

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) return NULL;

    qint64 size = file.size();
    if (size < IMAGE_START) return NULL;

    if (!file.seek(IMAGE_START)) return NULL;

    QByteArray data;

    char header[9] = {0};
    file.read(header, 8);

    data.append(header);

    // Verify PNG header
    if (data != IHDR) return NULL;

    // Begin seeking through PNG chunks
    while (!end) {
        // Calculate chunk length
        char len_data[4] = {0};
        file.read(len_data, 4);

        quint32 len = 0;
        len = len | (len_data[0] & 0xff) << 8 * 3;
        len = len | (len_data[1] & 0xff) << 8 * 2;
        len = len | (len_data[2] & 0xff) << 8 * 1;
        len = len | (len_data[3] & 0xff) << 8 * 0;

        data.append(len_data, 4);

        // Read chunk type
        char type_data[5] = {0};
        file.read(type_data, 4);

        QByteArray type(type_data);
        data.append(type);

        // Read data
        if (len > 0) {
            char chunk[len];
            file.read(chunk, len);
            data.append(chunk, len);
        }

        // Read CRC
        char crc[4];
        file.read(crc, 4);
        data.append(crc, 4);

        end = type == QByteArray("IEND");
    }

    return data;
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
            for (;; i++) {
                if (raw_text[i] == '\'') {
                    if (raw_text.length() > (i+1) && !raw_text[i+1].isLetterOrNumber()) {
                        break;
                    } else if (raw_text.length() <= (i+1)) {
                        break;
                    }
                }
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
            for (; raw_text[i] != '\n' && raw_text[i] != '\r'; i++) {
                s.append(raw_text[i]);
            }
            CONData *c = new CONData();
            c->text = s;
            list->append(c);
            s = "";
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

bool CONFile::writeFile(QString mount_path)
{
    QDir dir(mount_path);
    dir.cd(QString("Content/0000000000000000/%1/00000001").arg(RB_CODE));

    /*
    QString basename = this->filename.left(6) + "~";

    int counter = 1;
    for (; counter < 9; counter++) {
        if (!dir.exists(basename + QString::number(counter, 10)))
            break;
    }

    QString filename;

    if (dir.exists(basename + QString::number(counter, 10))) {
        QChar c;
        for (int i = 0; i < letters.length(); i++) {
            c = letters[i];
            if (!dir.exists(basename + QString(c)))
                break;
        }

        filename = basename + QString(c);
    } else {
        filename = basename + QString::number(counter, 10);
    }

    */
    QString filename = this->filename;
    bool res = QFile::copy(this->local_filepath, dir.absoluteFilePath(filename));

    if (res) this->local_filepath = dir.absoluteFilePath(filename);
    return res;
}

bool CONFile::overwriteFile(QString mount_path, QString existing_filepath)
{
    QDir dir(mount_path);
    dir.cd(QString("Content/0000000000000000/%1/00000001").arg(RB_CODE));

    if (dir.exists(existing_filepath)) {
        QFile::remove(existing_filepath);
    }

    bool res = QFile::copy(this->local_filepath, existing_filepath);
    if (res) this->local_filepath = existing_filepath;
    return res;
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
