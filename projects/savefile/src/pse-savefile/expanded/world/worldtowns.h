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

constexpr var8 townCount = 11;     ///< Towns tracked as fly destinations.
constexpr var8 townByteCount = 2;  ///< 5 bits of 16 unused

/**
 * @brief "Visited" flags for towns -- which fly destinations are unlocked.
 *
 * A @ref visitedTowns bool array of @ref townCount flags; setting one unlocks that
 * town as a Fly destination. QML count/at/set access. Standard expanded-node
 * convention (see SaveFileExpanded).
 *
 * @see World.
 */
class SAVEFILE_AUTOPORT WorldTowns : public QObject
{
  Q_OBJECT

public:
  WorldTowns(SaveFile* saveFile = nullptr);
  virtual ~WorldTowns();

  void load(SaveFile* saveFile = nullptr); ///< Expand the visited-town flags from the save.
  void save(SaveFile* saveFile);           ///< Flatten the visited-town flags to the save.

  Q_INVOKABLE int townsCount();              ///< Number of town flags (townCount).
  Q_INVOKABLE bool townsAt(int ind);         ///< Is town @p ind visited (fly-unlocked)?
  Q_INVOKABLE void townsSet(int ind, bool val); ///< Set/clear visited for town @p ind.

signals:
  void visitedTownsChanged();

public slots:
  void reset();     ///< Clear all visited flags.
  void randomize(); ///< Randomize the visited flags.

public:
  bool visitedTowns[townCount]; ///< Per-town visited (fly-unlocked) flags.
};
