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

// Can't have a total byte count given scripts are a bit more complicated to
// read
constexpr var8 scriptCount = 97; ///< Number of per-map script-progress values.

/**
 * @brief Per-map script progress values (the in-progress state of each map's script).
 *
 * Each map has a script "step" counter tracking how far its events have advanced;
 * this holds all @ref scriptCount of them as a @ref curScripts array. Unlike the
 * boolean event/missable flags these are small integers, so the QML accessors take
 * and return @c int. Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see World, AreaMap::curMapScript (the current map's live script index).
 */
class SAVEFILE_AUTOPORT WorldScripts : public QObject
{
  Q_OBJECT

public:
  WorldScripts(SaveFile* saveFile = nullptr);
  virtual ~WorldScripts();

  void load(SaveFile* saveFile = nullptr); ///< Expand the script values from the save.
  void save(SaveFile* saveFile);           ///< Flatten the script values to the save.

  Q_INVOKABLE int scriptsCount();              ///< Number of script values (scriptCount).
  Q_INVOKABLE int scriptsAt(int ind);          ///< Script value for map index @p ind.
  Q_INVOKABLE void scriptsSet(int ind, int val); ///< Set the script value for @p ind.

signals:
  void curScriptsChanged();

public slots:
  void reset();     ///< Zero all script values.
  void randomize(); ///< Randomize the script values.

public:
  var16 curScripts[scriptCount]; ///< Per-map script-progress values.
};
