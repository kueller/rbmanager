#ifndef CONFILE_H
#define CONFILE_H

#include <QDir>
#include <QList>
#include <QFile>
#include <QTreeWidget>
#include <QProcess>

#define METADATA_START 0xd000
#define MIDI_START 0xe000

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
    QTreeWidgetItem *item;
    QList<CONData *> *raw_data;

    CONFile(QString);
    ~CONFile();

    QString songName();
    QString artist();
    QString album();
    static bool isCONFile(QString filepath);
    static bool verifyDirectoryStructure(QString mount_path);
    static QString songsDirectory(QString mount_path);
    void writeFile(QString mount_path);
    void overwriteFile(QString mount_path, QString existing_filepath);

private:
    QString readMetadata(QString filepath);
    int parseMetadata(QString raw_text, QList<CONData *> *list, int i);
};

bool verify_directory_structure(QString mount_path);
int parse_metadata(QString raw_text, QList<CONData *> *list, int i);

#endif // CONFILE_H
