/*
  * Copyright 2020 Fairy Fox
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
#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include <pse-common/types.h>
#include "savefile_autoport.h"

#include "expanded/savefileexpanded.h"

class SaveFileExpanded;
class SaveFileIterator;
class SaveFileToolset;

constexpr var16 SAV_DATA_SIZE{0x8000}; ///< Size of a Gen 1 save in bytes (32 KB).

/**
 * @brief One loaded save: the raw 32 KB bytes, their expanded object tree, and
 *        the tools that move between them.
 *
 * SaveFile is the hinge of the whole app. It holds three things: the raw byte
 * buffer @ref data (the on-disk source of truth), the @ref dataExpanded object
 * tree that the QML UI actually edits, and a @ref toolset for byte-level access.
 * Its verbs move data between the two representations -- expandData() parses raw
 * -> objects, flattenData() writes objects -> raw (only the strictly-necessary
 * bytes). It is exposed to QML through FileManagement as `brg.file.data`.
 *
 * @see SaveFileExpanded (the object tree), SaveFileToolset / SaveFileIterator
 *      (byte access), and [the system map](../../../../notes/systems/overview.md).
 */
class SAVEFILE_AUTOPORT SaveFile : public QObject
{
  Q_OBJECT

  /// The expanded, editable object tree. QML traverses in from here.
  Q_PROPERTY(SaveFileExpanded* dataExpanded MEMBER dataExpanded NOTIFY dataExpandedChanged)

public:
  /// Create a blank save file and a blank expanded save file.
  SaveFile(QObject *parent = nullptr);
  virtual ~SaveFile();

  /**
   * @brief Returns a unique iterator that's setup to iterate over the raw sav
   *        file data.
   *
   * Heavily used by the file expansion to iterate and expand or flatten data.
   * @warning The iterator has to be deleted by the receiver or it will cause a
   *          memory leak.
   */
  SaveFileIterator* iterator();

  /**
   * @brief Change-out the save file.
   *
   * Re-Expand the new save file, overwriting prior expansion, unless marked
   * silent.
   * @param data   New raw save bytes to adopt.
   * @param silent When true, skip re-expanding the new data.
   */
  void setData(var8* data, bool silent = false);

signals:
  /// SAV file has changed and it's expansion replaced with new SAV data.
  void dataChanged(var8* data);

  /// SAV file has changed but the old expansion has not been replaced with
  /// exxpansion of new data.
  void dataExpandedChanged(SaveFileExpanded* expanded);

public slots:
  /// Empty this save file to zero's.
  /// Re-Expand the empty save file, overwriting prior expansion, unless marked
  /// silent.
  void resetData(bool silent = false);

  /// Flatten expansion back to the save file, overwriting it's current contents
  /// with only data that's strictly nesesary. A critical rule.
  void flattenData();

  /// Replace expansion with new expansion of current sav file.
  void expandData();

  /// Erase expansion data, this makes expansion data act like a new file
  /// but save file contents are preserved.
  void eraseExpansion();

  /// Fully randomizes the expansion data, doesn't change save file data.
  /// This tries to give fun and playable randomization. Due to the complexity of
  /// Gen 1 Games there are limits on randomization. The idea is to randomize
  /// everything we can and still allow you to jump in and play right away with
  /// Your Psychic Traded Pikachu named Bob that uses Ice Beam and Splash.
  void randomizeExpansion();

public:
  /// Actual SAV Data, a raw internal binary copy of the file.
  var8* data = nullptr;

  /// Expanded SAV data to be readable and more usable.
  SaveFileExpanded* dataExpanded = nullptr;

  /// Tools to operate directly on the raw sav file data.
  SaveFileToolset* toolset = nullptr;
};
