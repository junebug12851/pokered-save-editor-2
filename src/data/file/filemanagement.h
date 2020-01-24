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
#ifndef FILEMANAGEMENT2_H
#define FILEMANAGEMENT2_H

//#include <QtCore/qglobal.h>
#include <QFile>
#include <QSettings>

#include "../../common/types.h"

class SaveFile;

constexpr var8 MAX_RECENT_FILES{5};

class FileManagement : public QObject
{
  Q_OBJECT

public:
  Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathChanged)
  Q_PROPERTY(QList<QString> recentFiles READ getRecentFiles RESET clearRecentFiles NOTIFY recentFilesChanged)
  Q_PROPERTY(QString recentFile READ getRecentFile WRITE addRecentFile NOTIFY recentFilesChanged STORED false)
  Q_PROPERTY(SaveFile* data_ MEMBER data NOTIFY dataChanged)

  FileManagement(QObject* parent = nullptr);
  virtual ~FileManagement();

  QString getPath();
  QString getRecentFile(var8 index = 0);
  QList<QString> getRecentFiles();

  SaveFile* data = nullptr;

  static const QString KEY_RECENT_FILES;
  static const QString KEY_LAST_FILE;

signals:
  void pathChanged(QString newPath, QString oldPath);
  void recentFilesChanged(QList<QString> files);
  void dataChanged();

public slots:
  void reset();

  void newFile();
  void openFile();
  void openFileRecent(var8 index);
  void reopenFile();

  void addRecentFile(QString path);
  void setPath(QString path);

  void saveFile();
  void saveFileAs();
  void saveFileCopy();

  void wipeUnusedSpace();
  void clearRecentFiles();

private:
  void processRecentFileChanges();

  QString openFileDialog(QString title);
  QString saveFileDialog(QString title);

  var8* readSaveData(QString filePath);
  void writeSaveData(QString filePath, var8* data);

  void expandRecentFiles(QString files);

  QString path;
  QList<QString> recentFiles;
  QSettings settings;
};

#endif // FILEMANAGEMENT2_H
