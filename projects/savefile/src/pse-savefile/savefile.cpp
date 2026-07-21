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

/**
 * @file savefile.cpp
 * @brief Implementation of SaveFile -- the raw<->expanded lifecycle
 *        (expand/flatten/reset/randomize). See savefile.h for the documented API.
 */
#include "savefile.h"
#include "./expanded/savefileexpanded.h"
#include "./savefileiterator.h"
#include "./savefiletoolset.h"

SaveFile::SaveFile(QObject* parent)
  : QObject(parent)
{
  // One-Time Init Data in order of dependencies
  // Since the sav file is a primitive array we have to clear it first to zeroes
  data = new var8[SAV_DATA_SIZE];
  memset(data, 0, SAV_DATA_SIZE);

  // Create toolset 2nd and then dataExpanded
  // dataExpanded will perform an initial load from sav data which is why
  // we had to clear it first up there, dataExpanded also depends on toolset
  toolset = new SaveFileToolset(this);
  dataExpanded = new SaveFileExpanded(this);

  // Perform post-init stuff including notifying other code of data changes
  resetData();
}

SaveFile::~SaveFile()
{
  // Erase backwards from creation order
  dataExpanded->deleteLater();
  delete toolset;
  delete[] data;
}

SaveFileIterator* SaveFile::iterator()
{
  return new SaveFileIterator(this);
}

void SaveFile::resetData(bool silent)
{
  memset(data, 0, SAV_DATA_SIZE);
  dataChanged(data);

  if(!silent) {
    expandData();
    dataExpandedChanged(dataExpanded);
  }
}

void SaveFile::setData(var8* data, bool silent)
{
  // Never copy from a null source. setData's callers obtain their buffer from
  // FileManagement::readSaveData(), which returns nullptr when a file can't be
  // opened or is too short to be a valid save. Dereferencing that here is a hard
  // crash (memcpy from address 0). Treat a null buffer as "no change" -- the
  // current in-memory save is left exactly as it was, and the caller surfaces the
  // failure to the user. (Belt-and-braces: callers also guard before calling.)
  if(data == nullptr)
    return;

  memcpy(this->data, data, SAV_DATA_SIZE);
  dataChanged(data);

  if(!silent) {
    expandData();
    dataExpandedChanged(dataExpanded);
  }
}

void SaveFile::flattenData()
{
  dataExpanded->save(this);
}

void SaveFile::expandData()
{
  dataExpanded->load(this);
}

void SaveFile::eraseExpansion()
{
  dataExpanded->reset();
}

void SaveFile::randomizeExpansion()
{
  dataExpanded->randomize();
}
