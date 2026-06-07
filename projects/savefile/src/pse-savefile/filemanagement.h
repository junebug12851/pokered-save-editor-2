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
#pragma once
#include <QList>
#include <QFile>
#include <QSettings>

#include <pse-common/types.h>
#include "savefile_autoport.h"

#include "savefile.h"

class SaveFile;

constexpr var8 MAX_RECENT_FILES{5};                       ///< How many recent paths to remember.
constexpr const char* KEY_RECENT_FILES = "recentFiles";   ///< QSettings key for the recent-files list.
constexpr const char* KEY_LAST_FILE = "lastFile";         ///< QSettings key for the most-recent path.

/**
 * @brief Owns the on-disk side of a save: the current path, the recent-files
 *        list, and the live SaveFile.
 *
 * This is the top of the savefile layer that the UI talks to -- QML reaches it as
 * `brg.file`. It handles the file verbs (new/open/save/reopen, plus "save as" and
 * "save copy"), maintains a persisted recent-files list via QSettings, and reads
 * raw bytes off disk into a SaveFile (and writes them back). The actual byte
 * parsing lives in SaveFile / SaveFileToolset; this class is the I/O and
 * file-lifecycle controller around it.
 *
 * @see SaveFile (`data`), and [the data lifecycle](../../../../notes/systems/overview.md).
 */
class SAVEFILE_AUTOPORT FileManagement : public QObject
{
  Q_OBJECT

  /// Current file path. Setting it triggers a load via setPath().
  Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathChanged)
  /// The live save. QML reads through this as `brg.file.data`.
  Q_PROPERTY(SaveFile* data MEMBER data NOTIFY dataChanged)
  /// Plain-English description of the most recent failed load (the primary message).
  Q_PROPERTY(QString lastErrorMessage READ getLastErrorMessage NOTIFY loadError)
  /// The real, one-line technical detail behind the failure (the OS/Qt file error
  /// string, or the actual size mismatch). Shown small/secondary -- never as the
  /// primary message, and not a made-up code.
  Q_PROPERTY(QString lastErrorDetail READ getLastErrorDetail NOTIFY loadError)

public:
  FileManagement(QObject* parent = nullptr);
  virtual ~FileManagement();

  QString getLastErrorMessage(); ///< @see lastErrorMessage property.
  QString getLastErrorDetail();  ///< @see lastErrorDetail property.

  QString getPath();                              ///< Current file path.
  Q_INVOKABLE QString getRecentFile(int index = 0); ///< Recent path at @p index (0 = most recent).
  QList<QString> getRecentFiles();                ///< The whole recent-files list.

  /// The live save file these operations load into / save from.
  SaveFile* data = nullptr;

  Q_INVOKABLE int recentFilesCount();             ///< How many recent files are currently remembered.
  Q_INVOKABLE int recentFilesMax();               ///< The cap (MAX_RECENT_FILES).
  Q_INVOKABLE void recentFilesSwap(int from, int to); ///< Reorder the recent list (e.g. drag to reorder).
  Q_INVOKABLE void recentFilesRemove(int ind);    ///< Drop one entry from the recent list.

signals:
  void pathChanged(QString newPath, QString oldPath); ///< The active path changed.
  void recentFilesChanged(QList<QString> files);      ///< The recent-files list changed.
  void dataChanged();                                 ///< The live SaveFile was replaced.
  /// A load failed on a file that exists (unreadable / truncated). The app layer
  /// reacts by showing the file-error screen; read lastErrorMessage (plain) and
  /// lastErrorDetail (the real technical one-liner).
  void loadError();

public slots:
  void reset();          ///< Clear path + data back to a blank starting state.

  void newFile();        ///< Start a fresh blank save.
  bool openFile();       ///< Prompt for and open a save. @return true on success.
  bool openFileRecent(int index); ///< Open an entry from the recent-files list. @return true on success.
  void reopenFile();     ///< Reload the current path from disk, discarding edits.

  void addRecentFile(QString path); ///< Push a path onto the recent list (de-duped, capped).
  void setPath(QString path);       ///< Set the active path (backs the `path` property).

  bool saveFile();       ///< Save to the current path. @return true on success.
  bool saveFileAs();     ///< Prompt for a new path and save there. @return true on success.
  bool saveFileCopy();   ///< Save a copy elsewhere without changing the active path.

  void wipeUnusedSpace(); ///< Zero out save regions that aren't meaningfully used.
  void clearRecentFiles(); ///< Forget the entire recent-files list.

private:
  void processRecentFileChanges(); ///< Persist + emit after the recent list changes.
  void pruneRecentFiles(); ///< Drop recent entries that can't be opened for reading (startup cleanup).

  QString openFileDialog(QString title); ///< Native open dialog; returns the chosen path.
  QString saveFileDialog(QString title); ///< Native save dialog; returns the chosen path.

  /// Shared open path for openFile/openFileRecent/reopenFile: read @p filePath,
  /// adopt it into the live save on success, or record the failure and emit
  /// loadError() on failure (without touching the current save). @return true on success.
  bool loadData(const QString& filePath);
  void reportLoadError(); ///< Build lastErrorMessage from lastError and emit loadError().

  var8* readSaveData(QString filePath);            ///< Read raw save bytes from disk. Sets lastError; nullptr on failure.
  void writeSaveData(QString filePath, var8* data); ///< Write raw save bytes to disk.

  void expandRecentFiles(QString files); ///< Parse the persisted recent-files blob into the list.

  QString path;               ///< Backing store for the `path` property.
  QList<QString> recentFiles; ///< In-memory recent-files list (persisted via QSettings).
  QSettings settings;         ///< Persistent storage for path + recent files.

  /// Why a load failed -- internal only; selects which plain-English message to
  /// show. Deliberately NOT surfaced as a user-facing numeric "error code".
  enum FileLoadError { LoadErrorNone, LoadErrorCannotOpen, LoadErrorTooShort };

  FileLoadError lastError = LoadErrorNone; ///< Result of the most recent read attempt.
  QString lastErrorMessage;                ///< Plain-English text for lastError (built in reportLoadError()).
  QString lastErrorDetail;                 ///< Real one-line technical detail (set in readSaveData()).
};
