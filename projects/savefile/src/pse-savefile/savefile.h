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
#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include <pse-common/types.h>
#include "savefile_autoport.h"

// Full include so QML can traverse brg.file.data.dataExpanded.* . A forward
// -declared QObject pointer in a Q_PROPERTY reads as `undefined` in QML.
// See notes/reference/qt6-patterns.md.
#include "expanded/savefileexpanded.h"

class SaveFileExpanded;
class SaveFileIterator;
class SaveFileToolset;

constexpr var16 SAV_DATA_SIZE{0x8000};

class SAVEFILE_AUTOPORT SaveFile : public QObject
{
  Q_OBJECT

  Q_PROPERTY(SaveFileExpanded* dataExpanded MEMBER dataExpanded NOTIFY dataExpandedChanged)

public:
  // Create a blank save file and a blank expanded save file
  SaveFile(QObject *parent = nullptr);
  virtual ~SaveFile();

  // Returns a unique iterator that's setup to iterate over the raw sav file
  // data. Heavily used by the file expansion to iterate and expand or flatten
  // data.
  // The iterator has to be deleted by the receiver or it will cause a memory
  // leak
  SaveFileIterator* iterator();

  // Change-out the save file
  // Re-Expand the new save file, overwriting prior expansion, unless marked
  // silent
  void setData(var8* data, bool silent = false);

signals:
  // SAV file has changed and it's expansion replaced with new SAV data
  void dataChanged(var8* data);

  // SAV file has changed but the old expansion has not been replaced with
  // exxpansion of new data
  void dataExpandedChanged(SaveFileExpanded* expanded);

public slots:
  // Empty this save file to zero's
  // Re-Expand the empty save file, overwriting prior expansion, unless marked
  // silent
  void resetData(bool silent = false);

  // Flatten expansion back to the save file, overwriting it's current contents
  // with only data that's strictly nesesary. A critical rule.
  void flattenData();

  // Replace expansion with new expansion of current sav file
  void expandData();

  // Erase expansion data, this makes expansion data act like a new file
  // but save file contents are preserved
  void eraseExpansion();

  // Fully randomizes the expansion data, doesn't change save file data
  // This tries to give fun and playable randomization. Due to the complexity of
  // Gen 1 Games there are limits on randomization. The idea is to randomize
  // everything we can and still allow you to jump in and play right away with
  // Your Psychic Traded Pikachu named Bob that uses Ice Beam and Splash
  void randomizeExpansion();

public:
  // Actual SAV Data, a raw internal binary copy of 