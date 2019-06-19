#include <QFileDialog>

#include "rawsavedata.h"
#include "filemanagement.h"

FileManagement::FileManagement(QObject *parent)
    : QObject(parent), _path("")
{
    // Initially set to no open file (New File)
    this->setPath("");
    this->expandRecentFiles(settings.value(KEY_RECENT_FILES, QString("")).toString());
}

RawSaveData *FileManagement::data()
{
    return &this->_data;
}

QString FileManagement::path()
{
    return this->_path;
}

// Replace array with contents shifted down, oldest one removed, newest one
// most recent
void FileManagement::addRecentFile(QString path)
{
    QString tmp[MAX_RECENT_FILES];

    tmp[0] = path;
    for(int i = 1; i < MAX_RECENT_FILES - 1; i++) {
        tmp[i] = this->_recentFiles[i];
    }

    // Inform to update
    this->setRecentFiles(tmp);
}

QString FileManagement::recentFile(int index)
{
    return this->_recentFiles[index];
}

void FileManagement::clearRecentFiles()
{
    for(int i = 0; i < MAX_RECENT_FILES; i++) {
        this->_recentFiles[i] = "";
    }

    settings.setValue(KEY_RECENT_FILES, "");
    this->recentFilesChanged(this->_recentFiles);
}

QString *FileManagement::recentFiles()
{
    return this->_recentFiles;
}

void FileManagement::setRecentFiles(QString files[MAX_RECENT_FILES])
{
    // Don't bother if they're the same
    if(files == this->_recentFiles)
        return;

    // Settings has a clunky array saving feature
    // For simplicity we concat all files into a string seperated by ";"
    // This saves it quickly as one key with the disadvantage that Linux/Unix
    // paths and files cannot have a ";" in them which I imagine to be very rare
    QString toSettings = "";

    for(int i = 0; i < MAX_RECENT_FILES; i++) {
        this->_recentFiles[i] = files[i];
        toSettings += files[i] + ";";
    }

    // Remove last ";"
    toSettings.resize(toSettings.size() - 1);

    settings.setValue(KEY_RECENT_FILES, toSettings);
    this->recentFilesChanged(this->_recentFiles);
}

void FileManagement::newFile()
{
    this->setPath("");
    this->data()->resetData();
}

void FileManagement::openFile()
{
    QString file = this->openFileDialog("Open Save File");
    if(file == "")
        return;

    quint8* data = this->readSaveData(file);
    this->data()->setData(data); // Copies data out of array (Safe to delete)
    this->setPath(file);
    delete data; // Very important with readSaveData
}

void FileManagement::openFileRecent(int index)
{
    QString file = this->recentFile(index);
    quint8* data = this->readSaveData(file);
    this->data()->setData(data);
    this->setPath(file);
}

void FileManagement::reopenFile()
{
    // Erase data if path is empty
    if(this->path() == "") {
        this->data()->resetData();
        return;
    }

    // Otherwise destroy current working copy with copy from disk
    this->readSaveData(this->path());
}

void FileManagement::saveFile()
{
    if(this->path() == "") {
        this->saveFileAs();
        return;
    }

    this->writeSaveData(this->path(), this->data()->data());
}

void FileManagement::saveFileAs()
{
    QString filename = this->saveFileDialog("Save File As...");
    if(filename == "")
        return;

    this->writeSaveData(filename, this->data()->data());
    this->setPath(filename);
}

void FileManagement::saveFileCopy()
{
    QString filename = this->saveFileDialog("Save Copy As...");
    if(filename == "")
        return;

    this->writeSaveData(filename, this->data()->data());
}

void FileManagement::wipeUnusedSpace()
{
    this->data()->resetData(true);
}

QString FileManagement::openFileDialog(QString title)
{
    return QFileDialog::getOpenFileName(
                nullptr,
                title,
                this->path(),
                "Save Files (*.sav);;All Files (*)");
}

QString FileManagement::saveFileDialog(QString title)
{
    return QFileDialog::getSaveFileName(
                nullptr,
                title,
                this->path(),
                "Save Files (*.sav);;All Files (*)");
}

// Pointer has to be deleted to prevent memory leaks
quint8 *FileManagement::readSaveData(QString filePath)
{
    // Load up file in system
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    // Read in raw bytes signed
    char* rawSaveData = new char[SAV_DATA_SIZE];
    in.readRawData(rawSaveData, SAV_DATA_SIZE);

    file.close();

    return reinterpret_cast<quint8*>(rawSaveData);
}

void FileManagement::writeSaveData(QString filePath, quint8 *data)
{
    // Load up file in system
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    // Convert pointer over to a type needed for QDataStream and write file
    char* dataChar = reinterpret_cast<char*>(data);
    out.writeRawData(dataChar, SAV_DATA_SIZE);

    file.close();
}

// Simply expands recent files string into memory and notifies of the update
void FileManagement::expandRecentFiles(QString key)
{
    QStringList recentFiles = key.split(';');

    // Erase Array
    for(int i = 0; i < MAX_RECENT_FILES; i++) {
        this->_recentFiles[i] = "";
    }

    // Add in recent files no longer than provided string and max capacity
    for(int i = 0; i < MAX_RECENT_FILES && i < recentFiles.size(); i++) {
        this->_recentFiles[i] = recentFiles[i];
    }

    this->recentFilesChanged(this->_recentFiles);
}

void FileManagement::setPath(QString path)
{
    // Stop here if they're the same
    if(path == this->_path)
        return;

    //Change paths, notify, add to recentFiles
    QString oldPath = this->_path;
    this->_path = path;
    this->pathChanged(path, oldPath);

    // Go no further if path is empty, we don't need to save it
    if(path == "")
        return;

    this->addRecentFile(path);
    settings.setValue(KEY_LAST_FILE, path);
}
