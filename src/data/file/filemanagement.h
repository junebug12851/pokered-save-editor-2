#ifndef FILEMANAGEMENT_H
#define FILEMANAGEMENT_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QFile>
#include <QSettings>

#include "savefile.h"
#include "../../common/types.h"

constexpr var8 MAX_RECENT_FILES{5};

class FileManagement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(SaveFile* data READ data)
    Q_PROPERTY(QList<QString>* recentFiles READ recentFiles RESET clearRecentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QString recentFile READ recentFile NOTIFY recentFilesChanged STORED false)

public:
    // Construct
    explicit FileManagement(QObject *parent = nullptr);

    // Save Data and Path
    SaveFile* data();
    QString path();

    // Manage Recent Files
    QString recentFile(var8 index = 0);
    QList<QString>* recentFiles();

    static const QString KEY_RECENT_FILES;
    static const QString KEY_LAST_FILE;

signals:
    void pathChanged(QString path, QString oldPath);
    void recentFilesChanged(QList<QString>* files);

public slots:
    // Open/Save Files
    void newFile();
    void openFile();
    void openFileRecent(var8 index);
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
    var8* readSaveData(QString filePath);
    void writeSaveData(QString filePath, var8* data);

    // Internal paths handling
    void expandRecentFiles(QString files);
    void setPath(QString path);

    // Internal variables
    SaveFile* _data = new SaveFile();
    QString _path = "";
    QList<QString>* _recentFiles = new QList<QString>();
    QSettings* settings = new QSettings();
};

#endif // FILEMANAGEMENT_H
