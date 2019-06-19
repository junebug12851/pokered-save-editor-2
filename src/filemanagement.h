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
    Q_PROPERTY(QString* recentFiles READ recentFiles WRITE setRecentFiles RESET clearRecentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QString recentFile READ recentFile WRITE addRecentFile NOTIFY recentFilesChanged STORED false)

public:
    // Construct
    explicit FileManagement(QObject *parent = nullptr);

    // Save Data and Path
    RawSaveData* data();
    QString path();

    // Manage Recent Files
    void addRecentFile(QString path);
    QString recentFile(int index = 0);

    void clearRecentFiles();
    QString* recentFiles();
    void setRecentFiles(QString files[MAX_RECENT_FILES]);

    // Open/Save Files
    void newFile();
    void openFile();
    void openFileRecent(int index);
    void reopenFile();

    void saveFile();
    void saveFileAs();
    void saveFileCopy();

    void wipeUnusedSpace();

signals:
    void pathChanged(QString path, QString oldPath);
    void recentFilesChanged(QString files[MAX_RECENT_FILES]);

private:
    // Internal File Dialog Handling
    QString openFileDialog(QString title);
    QString saveFileDialog(QString title);

    // Internal Save/Load Handling
    quint8* readSaveData(QString filePath);
    void writeSaveData(QString filePath, quint8* data);

    // Internal paths handling
    void expandRecentFiles(QString key);
    void setPath(QString path);

    // Internal variables
    RawSaveData _data;
    QString _path;
    QString _recentFiles[MAX_RECENT_FILES];
    QSettings settings;
};

#endif // FILEMANAGEMENT_H
