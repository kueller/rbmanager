#ifndef CONFILE_H
#define CONFILE_H

#include <QDir>
#include <QList>
#include <QFile>
#include <QTreeWidget>
#include <QProcess>
#include <QByteArray>

#define ENTRY_ID 0x340
#define FILE_TABLE_BLOCK_COUNT 0x37c
#define FILE_TABLE_BLOCK 0x37e
#define BLOCK_START 0xc000
#define IMAGE_START 0x171a
#define MIDI_START 0xe000

#define BLOCK_SIZE 0x1000
#define CON_FILENAME_LEN 0x28

class CONData
{

public:
    QString text;
    QList<CONData *> *list;

    CONData();
    ~CONData();
};

class CONFile
{

public:
    QString filename;
    QString local_filepath;
    QString dta;
    QTreeWidgetItem *item;
    QList<CONData *> *raw_data;
    QByteArray thumbnail;

    CONFile(QString);
    ~CONFile();

    QString songName();
    QString artist();
    QString album();
    QString year();
    static bool isCONFile(QString filepath);
    static bool verifyDirectoryStructure(QString mount_path);
    static QString songsDirectory(QString mount_path);
    bool writeFile(QString mount_path);
    bool overwriteFile(QString mount_path, QString existing_filepath);

    QString genericFind(QString query);
    QString author();
private:
    QString readMetadata(QString filepath);
    int parseMetadata(QString raw_text, QList<CONData *> *list, int i);
    QByteArray readThumbnail(QString filepath);
    quint32 getFileTypeOffset(QString filepath, QString extension);
    quint32 blockToOffset(quint32 block_start, quint32 entry_id);
};

bool verify_directory_structure(QString mount_path);
int parse_metadata(QString raw_text, QList<CONData *> *list, int i);

#endif // CONFILE_H
