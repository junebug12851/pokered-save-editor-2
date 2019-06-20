#ifndef FILEMANAGEMENT_H
#define FILEMANAGEMENT_H

#define MAX_RECENT_FILES 5

#define KEY_RECENT_FILES "recentFiles"
#define KEY_LAST_FILE "lastFile"

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QFile>
#include <QSettings>

#include "rawsavedata.h"

class FileManagement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(RawSaveData* data READ data)
    Q_PROPERTY(QList<QString>* recentFiles READ recentFiles RESET clearRecentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QString recentFile READ recentFile NOTIFY recentFilesChanged STORED false)

public:
    // Construct
    explicit FileManagement(QObject *parent = nullptr);

    // Save Data and Path
    RawSaveData* data();
    QString path();

    // Manage Recent Files
    QString recentFile(int index = 0);
    QList<QString>* recentFiles();

signals:
    void pathChanged(QString path, QString oldPath);
    void recentFilesChanged(QList<QString>* files);

public slots:
    // Open/Save Files
    void newFile();
    void openFile();
    void openFileRecent(int index);
    void reopenFile();

    void saveFile();
    void saveFileAs();
    void saveFileCopy();

    void wipeUnusedSpace();
    void clearRecentFiles();

private:
    // Internal management of recent files
    void addRecentFile(QString path);
    void processRecentFileChanges();

    // Internal File Dialog Handling
    QString openFileDialog(QString title);
    QString saveFileDialog(QString title);

    // Internal Save/Load Handling
    quint8* readSaveData(QString filePath);
    void writeSaveData(QString filePath, quint8* data);

    // Internal paths handling
    void expandRecentFiles(QString files);
    void setPath(QString path);

    // Internal variables
    RawSaveData _data;
    QString _path;
    QList<QString> _recentFiles;
    QSettings settings;
};

#endif // FILEMANAGEMENT_H
