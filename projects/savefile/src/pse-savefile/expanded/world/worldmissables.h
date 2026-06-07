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
#include <QObject>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;

// There's actually significantly more missable bits, 256 in total forming
// 32 bytes. But given they are all unused it makes no sense to load entire
// unused bytes
constexpr var8 missableCount = 228;    ///< Missable-sprite flags actually used.
constexpr var8 missableByteCount = 29; ///< 4 bits unused of 232

/**
 * @brief Visibility flags for "missable" sprites (one-time NPCs/items on maps).
 *
 * Each SpriteData missable points at a bit here that decides whether it currently
 * renders -- e.g. an item ball that disappears once taken. Presented as a flat
 * @ref missables bool array (only the used bits; see the count note). QML
 * count/at/set access. Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see World, SpriteData (its `missableIndex` points here).
 */
class SAVEFILE_AUTOPORT WorldMissables : public QObject
{
  Q_OBJECT

public:
  WorldMissables(SaveFile* saveFile = nullptr);
  virtual ~WorldMissables();

  void load(SaveFile* saveFile = nullptr); ///< Expand the missable flags from the save.
  void save(SaveFile* saveFile);           ///< Flatten the missable flags to the save.

  Q_INVOKABLE int missablesCount();              ///< Number of missable flags.
  Q_INVOKABLE bool missablesAt(int ind);         ///< Is missable @p ind set (shown/hidden)?
  Q_INVOKABLE void missablesSet(int ind, bool val); ///< Set/clear missable @p ind.

signals:
  void missablesChanged();

public slots:
  void reset();     ///< Clear all missable flags.
  void randomize(); ///< Randomize the missable flags.

public:
  bool missables[missableCount]; ///< Per-missable visibility flags.
};
