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
    Q_PROPERTY(QString* recentFiles READ recentFiles RESET clearRecentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QString recentFile READ recentFile NOTIFY recentFilesChanged STORED false)

public:
    // Construct
    explicit FileManagement(QObject *parent = nullptr);

    // Save Data and Path
    RawSaveData* data();
    QString path();

    // Manage Recent Files
    QString recentFile(int index = 0);

    void clearRecentFiles();
    QString* recentFiles();

    // Open/Save Files
    Q_INVOKABLE void newFile();
    Q_INVOKABLE void openFile();
    Q_INVOKABLE void openFileRecent(int index);
    Q_INVOKABLE void reopenFile();

    Q_INVOKABLE void saveFile();
    Q_INVOKABLE void saveFileAs();
    Q_INVOKABLE void saveFileCopy();

    Q_INVOKABLE void wipeUnusedSpace();

signals:
    void pathChanged(QString path, QString oldPath);
    void recentFilesChanged(QString files[MAX_RECENT_FILES]);

private:
    // Internal management of recent files
    void addRecentFile(QString path);
    void setRecentFiles(QString files[MAX_RECENT_FILES]);

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
