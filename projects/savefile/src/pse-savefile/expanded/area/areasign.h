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
#include <QObject>
#include <QVector>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class SignData;
class MapDBEntry;

constexpr var8 maxSigns = 16; ///< Maximum signs on a map.

/**
 * @brief The current map's list of signs.
 *
 * A variable-length list of SignData (up to @ref maxSigns) with QML add/remove/
 * swap/access. setTo()/randomize() rebuild the list from a chosen map's signs.
 * Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see SignData (a sign), Area, MapDBEntry.
 */
class SAVEFILE_AUTOPORT AreaSign : public QObject
{
  Q_OBJECT

public:
  AreaSign(SaveFile* saveFile = nullptr);
  virtual ~AreaSign();

  void load(SaveFile* saveFile = nullptr); ///< Expand the sign list from the save.
  void save(SaveFile* saveFile);           ///< Flatten the sign list to the save.

  Q_INVOKABLE int signCount();             ///< Number of signs.
  Q_INVOKABLE int signMax();               ///< Capacity (maxSigns).
  Q_INVOKABLE SignData* signAt(int ind);   ///< Sign @p ind (GC-protected return).
  Q_INVOKABLE void signSwap(int from, int to); ///< Reorder signs.
  Q_INVOKABLE void signRemove(int ind);    ///< Remove sign @p ind.
  Q_INVOKABLE void signNew();              ///< Add a fresh sign.

signals:
  void signsChanged();

public slots:
  void reset();                       ///< Empty the sign list.
  void randomize(MapDBEntry* mapData); ///< Randomize signs for @p mapData.
  void setTo(MapDBEntry* mapData);     ///< Rebuild the list from @p mapData's signs.

public:
  QVector<SignData*> signs; ///< The map's signs.
};
