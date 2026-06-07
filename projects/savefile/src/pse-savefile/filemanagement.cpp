/*
  * Copyright 2020 Twilight
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *   http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
*/

/**
 * @file filemanagement.cpp
 * @brief Implementation of FileManagement -- file new/open/save/reopen and the
 *        recent-files list (disk I/O around SaveFile). See filemanagement.h.
 */

#include <QFileDialog>

#include "./filemanagement.h"
#include "./savefile.h"
#include "./savefiletoolset.h"
//#include "../../../ui/window/mainwindow.h"

FileManagement::FileManagement(QObject* parent) : QObject(parent)
{
  data = new SaveFile();
  reset();
}

FileManagement::~FileManagement()
{
  data->deleteLater();
}

QString FileManagement::getPath()
{
  return path;
}

QString FileManagement::getRecentFile(int index)
{
  return recentFiles.at(index);
}

QList<QString> FileManagement::getRecentFiles()
{
  return QList<QString>(recentFiles);
}

int FileManagement::recentFilesCount()
{
  return recentFiles.size();
}

int FileManagement::recentFilesMax()
{
  return MAX_RECENT_FILES;
}

void FileManagement::recentFilesSwap(int from, int to)
{
  auto eFrom = recentFiles.at(from);
  auto eTo = recentFiles.at(to);

  recentFiles.replace(from, eTo);
  recentFiles.replace(to, eFrom);

  processRecentFileChanges();
}

void FileManagement::recentFilesRemove(int ind)
{
  recentFiles.removeAt(ind);
  processRecentFileChanges();
}

void FileManagement::reset()
{
  setPath("");
  expandRecentFiles(settings.value(KEY_RECENT_FILES, "").toString());
  pruneRecentFiles(); // silently drop entries that no longer exist / can't be read
  newFile();
}

void FileManagement::newFile()
{
  setPath("");
  data->resetData();
}

bool FileManagement::openFile()
{
  QString file{openFileDialog("Open Save File")};
  if(file == "")
    return false;

  // Read + adopt. On failure loadData() surfaces the error and leaves the current
  // save untouched; don't setPath (so a bad file isn't recorded as recent/current).
  if(!loadData(file))
    return false;

  setPath(file);
  return true;
}

bool FileManagement::openFileRecent(int index)
{
  QString file{getRecentFile(index)};

  // On failure, leave everything as-is; the error screen is raised by loadData().
  // Returning false lets the caller (e.g. the New File modal) keep itself open.
  if(!loadData(file))
    return false;

  setPath(file);
  return true;
}

void FileManagement::reopenFile()
{
  // Erase data if path is empty
  if(path == "") {
    data->resetData();
    return;
  }

  // Otherwise destroy current working copy with copy from disk. On failure the
  // current working copy is preserved (loadData() leaves the save untouched).
  loadData(path);
}

bool FileManagement::loadData(const QString& filePath)
{
  var8* newData{readSaveData(filePath)}; // sets lastError; nullptr on failure
  if(newData == nullptr) {
    reportLoadError();
    return false;
  }

  data->setData(newData); // Copies data out of array (Safe to delete)
  delete[] newData;        // Very important with readSaveData
  return true;
}

void FileManagement::reportLoadError()
{
  // Plain-English primary message only -- the real technical detail is carried
  // separately in lastErrorDetail (set by readSaveData) and shown as a secondary line.
  switch(lastError) {
    case LoadErrorCannotOpen:
      lastErrorMessage =
        "This save file couldn't be opened.\n\n"
        "It may be open in another program, or you may not have permission to "
        "read it. Check that the file still exists and try again.";
      break;

    case LoadErrorTooShort:
      lastErrorMessage =
        "This save file looks truncated or corrupted.\n\n"
        "It's too small to be a valid save, so nothing was loaded and the "
        "current file was left untouched.";
      break;

    default:
      lastErrorMessage = "This save file couldn't be loaded.";
      break;
  }

  loadError();
}

QString FileManagement::getLastErrorMessage()
{
  return lastErrorMessage;
}

QString FileManagement::getLastErrorDetail()
{
  return lastErrorDetail;
}

void FileManagement::addRecentFile(QString path)
{
  // Add to top
  recentFiles.prepend(path);

  // Process (This ensures everyhtign is formatted and cleaned up as expected)
  processRecentFileChanges();
}

void FileManagement::setPath(QString path)
{
  // Stop here if they're the same
  if(path == this->path)
    return;

  //Change paths, notify, add to recentFiles
  QString oldPath{this->path};
  this->path = path;
  pathChanged(path, oldPath);

  // Go no further if path is empty, we don't need to save it
  if(path == "")
    return;

  addRecentFile(path);
  settings.setValue(KEY_LAST_FILE, path);
}

bool FileManagement::saveFile()
{
  if(path == "") {
    return saveFileAs();
  }

  data->flattenData();
  writeSaveData(path, data->data);
  return true;
}

bool FileManagement::saveFileAs()
{
  QString filename{saveFileDialog("Save File As...")};
  if(filename == "")
    return false;

  data->flattenData();
  writeSaveData(filename, data->data);
  setPath(filename);
  return true;
}

bool FileManagement::saveFileCopy()
{
  QString filename{saveFileDialog("Save Copy As...")};
  if(filename == "")
    return false;

  data->flattenData();
  writeSaveData(filename, data->data);
  return true;
}

void FileManagement::wipeUnusedSpace()
{
  data->resetData(true);
}

void FileManagement::clearRecentFiles()
{
  recentFiles.clear();
  processRecentFileChanges();
}

void FileManagement::processRecentFileChanges()
{
  // Cleanup First make sure correct length and contains no
  // empty strings or strings with spaces or duplicate strings, etc...
  QList<QString> newList;

  for(var8 i{0}; i < recentFiles.size(); ++i) {
    QString file{recentFiles.at(i)};
    file = file.trimmed();
    if(file == "" || newList.contains(file))
      continue;

    newList.append(file);
    if(newList.size() > MAX_RECENT_FILES)
      break;
  }

  // Replace current list with newly formatted list
  recentFiles = newList;

  // Save
  QString compacted{newList.join(';')};
  settings.setValue(KEY_RECENT_FILES, compacted);

  // Notify
  recentFilesChanged(recentFiles);
}

void FileManagement::pruneRecentFiles()
{
  // Startup cleanup: silently drop any recent entry we can't even open for
  // reading (moved/deleted/renamed/permission). Intentional UX -- a gone file
  // just disappears from the list; if the user goes looking for it they'll have
  // to re-open it deliberately. Truncated-but-openable files are NOT removed
  // here; those surface a clear error only when actually opened.
  // (Named "prune", not "scrub" -- "scrub" already means wiping a save's unused
  // bytes in this app; see wipeUnusedSpace.)
  QList<QString> kept;

  for(const QString& file : recentFiles) {
    QFile probe(file);
    if(probe.open(QIODevice::ReadOnly)) {
      probe.close();
      kept.append(file);
    }
  }

  recentFiles = kept;
  processRecentFileChanges(); // re-format, persist, and notify
}

QString FileManagement::openFileDialog(QString title)
{
  QString curPath{path};

  if(curPath == "")
    curPath = settings.value(KEY_LAST_FILE, "").toString();

  return QFileDialog::getOpenFileName(
        nullptr,
        //(QWidget*)MainWindow::getInstance(), @TODO
        title,
        curPath,
        "Save Files (*.sav);;All Files (*)");
}

QString FileManagement::saveFileDialog(QString title)
{
  QString curPath{path};

  if(curPath == "")
    curPath = settings.value(KEY_LAST_FILE, "").toString();

  return QFileDialog::getSaveFileName(
        nullptr,
        //(QWidget*)MainWindow::getInstance(), @TODO
        title,
        curPath,
        "Save Files (*.sav);;All Files (*)");
}

var8* FileManagement::readSaveData(QString filePath)
{
  lastError = LoadErrorNone;
  lastErrorDetail.clear();

  // Load up file in system
  QFile file(filePath);
  if(!file.open(QIODevice::ReadOnly)) {
    lastError = LoadErrorCannotOpen;
    lastErrorDetail = file.errorString(); // real, one-line OS/Qt reason
    return nullptr;
  }

  // Must be AT LEAST a full save's worth of bytes -- not exactly. Larger files
  // are fine (only the first SAV_DATA_SIZE bytes are the Gen 1 save); anything
  // shorter is truncated / corrupted and is rejected before we read it.
  const qint64 fileSize{file.size()};
  if(fileSize < static_cast<qint64>(SAV_DATA_SIZE)) {
    lastError = LoadErrorTooShort;
    lastErrorDetail = QStringLiteral("File is %1 bytes; a save must be at least %2 bytes (32 KB).")
                        .arg(fileSize)
                        .arg(static_cast<qint64>(SAV_DATA_SIZE));
    file.close();
    return nullptr;
  }

  QDataStream in(&file);

  // Allocate and zero-fill first: defensive, so the buffer is never uninitialized
  // heap garbage even if a read somehow comes up short (a byte-fidelity hazard).
  char* rawSaveData{new char[SAV_DATA_SIZE]};
  memset(rawSaveData, 0, SAV_DATA_SIZE);

  // Read the first SAV_DATA_SIZE bytes (the save); ignore any trailing bytes.
  in.readRawData(rawSaveData, SAV_DATA_SIZE);

  file.close();

  return reinterpret_cast<var8*>(rawSaveData);
}

void FileManagement::writeSaveData(QString filePath, var8* data)
{
  // Re-issue checksums
  this->data->toolset->recalcChecksums();

  // Load up file in system
  QFile file(filePath);
  if(!file.open(QIODevice::WriteOnly))
    return;
  QDataStream out(&file);

  // Convert pointer over to a type needed for QDataStream and write file
  char* dataChar{reinterpret_cast<char*>(data)};
  out.writeRawData(dataChar, SAV_DATA_SIZE);

  file.close();
}

void FileManagement::expandRecentFiles(QString files)
{
  // Break apart string into paths
  // Manually add them in, otherwise they oddly get out of order
  QStringList recentFiles{files.split(';')};

  for(var8 i{0}; i < recentFiles.size(); ++i) {
      this->recentFiles.append(recentFiles[i]);
  }

  // Process, cleanup, and notify
  processRecentFileChanges();
}
