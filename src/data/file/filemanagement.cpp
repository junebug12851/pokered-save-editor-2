/*
  * Copyright 2020 June Hanabi
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

#include <QFileDialog>

#include "./filemanagement.h"
#include "./savefile.h"
#include "./savefiletoolset.h"
#include "../../../ui/window/mainwindow.h"

FileManagement::FileManagement(QObject* parent) : QObject(parent)
{
  data = new SaveFile();
  reset();
}

FileManagement::~FileManagement()
{
  delete data;
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

  var8* newData{readSaveData(file)};
  data->setData(newData); // Copies data out of array (Safe to delete)
  setPath(file);
  delete[] newData; // Very important with readSaveData
  return true;
}

void FileManagement::openFileRecent(int index)
{
  QString file{getRecentFile(index)};
  var8* newData{readSaveData(file)};
  data->setData(newData);
  setPath(file);
  delete[] newData;
}

void FileManagement::reopenFile()
{
  // Erase data if path is empty
  if(path == "") {
    data->resetData();
    return;
  }

  // Otherwise destroy current working copy with copy from disk
  var8* newData{readSaveData(path)};
  data->setData(newData);
  delete[] newData;
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

void FileManagement::saveFile()
{
  if(path == "") {
    saveFileAs();
    return;
  }

  writeSaveData(path, data->data);
}

void FileManagement::saveFileAs()
{
  QString filename{saveFileDialog("Save File As...")};
  if(filename == "")
    return;

  writeSaveData(filename, data->data);
  setPath(filename);
}

void FileManagement::saveFileCopy()
{
  QString filename{saveFileDialog("Save Copy As...")};
  if(filename == "")
    return;

  writeSaveData(filename, data->data);
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

QString FileManagement::openFileDialog(QString title)
{
  QString curPath{path};

  if(curPath == "")
    curPath = settings.value(KEY_LAST_FILE, "").toString();

  return QFileDialog::getOpenFileName(
        (QWidget*)MainWindow::getInstance(),
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
        (QWidget*)MainWindow::getInstance(),
        title,
        curPath,
        "Save Files (*.sav);;All Files (*)");
}

var8* FileManagement::readSaveData(QString filePath)
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

void FileManagement::writeSaveData(QString filePath, var8* data)
{
  // Re-issue checksums
  this->data->toolset->recalcChecksums();

  // Load up file in system
  QFile file(filePath);
  file.open(QIODevice::WriteOnly);
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
