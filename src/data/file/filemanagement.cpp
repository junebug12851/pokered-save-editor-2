#include <QFileDialog>
#include <QList>

#include "savefile.h"
#include "filemanagement.h"
#include "../../../ui/window/mainwindow.h"

FileManagement::FileManagement(QObject *parent)
    : QObject(parent)
{
    // Initially set to no open file (New File)
    setPath("");
    expandRecentFiles(settings->value(KEY_RECENT_FILES, QString("")).toString());
}

SaveFile *FileManagement::data()
{
    return _data;
}

QString FileManagement::path()
{
    return _path;
}

// Replace array with contents shifted down, oldest one removed, newest one
// most recent
void FileManagement::addRecentFile(QString path)
{
    // Add to top
    _recentFiles->prepend(path);

    // Process (This ensures everyhtign is formatted and cleaned up as expected)
    processRecentFileChanges();
}

QString FileManagement::recentFile(var8 index)
{
    return (*_recentFiles)[index];
}

void FileManagement::clearRecentFiles()
{
    _recentFiles->clear();
    processRecentFileChanges();
}

QList<QString>* FileManagement::recentFiles()
{
    return _recentFiles;
}

void FileManagement::processRecentFileChanges()
{
    // Cleanup First make sure correct length and contains no
    // empty strings or strings with spaces or duplicate strings, etc...
    QList<QString>* newList = new QList<QString>();
    for(var8 i{0}; i < _recentFiles->size(); ++i) {
        QString file{(*_recentFiles)[i]};
        file = file.trimmed();
        if(file == "" || newList->contains(file))
            continue;

        newList->append(file);
        if(newList->size() > MAX_RECENT_FILES)
            break;
    }

    // Replace current list with newly formatted list
    delete _recentFiles;
    _recentFiles = newList;

    // Save
    QString compacted{newList->join(';')};
    settings->setValue(KEY_RECENT_FILES, compacted);

    // Notify
    recentFilesChanged(_recentFiles);
}

void FileManagement::newFile()
{
    setPath("");
    data()->resetData();
}

void FileManagement::openFile()
{
    QString file{openFileDialog("Open Save File")};
    if(file == "")
        return;

    var8* newData{readSaveData(file)};
    data()->setData(newData); // Copies data out of array (Safe to delete)
    setPath(file);
    delete[] newData; // Very important with readSaveData
}

void FileManagement::openFileRecent(var8 index)
{
    QString file{recentFile(index)};
    var8* newData{readSaveData(file)};
    data()->setData(newData);
    setPath(file);
}

void FileManagement::reopenFile()
{
    // Erase data if path is empty
    if(path() == "") {
        data()->resetData();
        return;
    }

    // Otherwise destroy current working copy with copy from disk
    readSaveData(path());
}

void FileManagement::saveFile()
{
    if(path() == "") {
        saveFileAs();
        return;
    }

    writeSaveData(path(), data()->data());
}

void FileManagement::saveFileAs()
{
    QString filename{saveFileDialog("Save File As...")};
    if(filename == "")
        return;

    writeSaveData(filename, data()->data());
    setPath(filename);
}

void FileManagement::saveFileCopy()
{
    QString filename{saveFileDialog("Save Copy As...")};
    if(filename == "")
        return;

    writeSaveData(filename, data()->data());
}

void FileManagement::wipeUnusedSpace()
{
    data()->resetData(true);
}

QString FileManagement::openFileDialog(QString title)
{
    QString curPath{path()};
    if(curPath == "")
        curPath = settings->value(KEY_LAST_FILE, QString("")).toString();

    return QFileDialog::getOpenFileName(
                MainWindow::instance(),
                title,
                curPath,
                "Save Files (*.sav);;All Files (*)");
}

QString FileManagement::saveFileDialog(QString title)
{
    QString curPath{path()};
    if(curPath == "")
        curPath = settings->value(KEY_LAST_FILE, QString("")).toString();

    return QFileDialog::getSaveFileName(
                MainWindow::instance(),
                title,
                curPath,
                "Save Files (*.sav);;All Files (*)");
}

// Pointer has to be deleted to prevent memory leaks
var8 *FileManagement::readSaveData(QString filePath)
{
    // Load up file in system
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    // Read in raw bytes signed
    char* rawSaveData{new char[SAV_DATA_SIZE]};
    in.readRawData(rawSaveData, SAV_DATA_SIZE);

    file.close();

    return reinterpret_cast<var8*>(rawSaveData);
}

void FileManagement::writeSaveData(QString filePath, var8 *data)
{
    // Load up file in system
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    // Convert pointer over to a type needed for QDataStream and write file
    char* dataChar{reinterpret_cast<char*>(data)};
    out.writeRawData(dataChar, SAV_DATA_SIZE);

    file.close();
}

// Simply expands recent files string into memory and notifies of the update
void FileManagement::expandRecentFiles(QString key)
{
    // Break apart string into paths
    // Manually add them in, otherwise they oddly get out of order
    QStringList recentFiles{key.split(';')};
    for(var8 i{0}; i < recentFiles.size(); ++i) {
        _recentFiles->append(recentFiles[i]);
    }

    // Process, cleanup, and notify
    processRecentFileChanges();
}

void FileManagement::setPath(QString path)
{
    // Stop here if they're the same
    if(path == _path)
        return;

    //Change paths, notify, add to recentFiles
    QString oldPath{_path};
    _path = path;
    pathChanged(path, oldPath);

    // Go no further if path is empty, we don't need to save it
    if(path == "")
        return;

    addRecentFile(path);
    settings->setValue(KEY_LAST_FILE, path);
}

const QString FileManagement::KEY_RECENT_FILES = "recentFiles";
const QString FileManagement::KEY_LAST_FILE = "lastFile";
