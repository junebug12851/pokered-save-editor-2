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
#include <QVector>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class MapDBEntrySign;
struct TmpSignPos;

/**
 * @brief One sign on the current map: its tile position and text id.
 *
 * A small leaf in the area's sign list. Holds the sign's @ref x / @ref y tile
 * coordinates and the @ref txtId that selects which message it shows. Can be
 * populated from map DB data (setTo / setToAll) or randomized, with `...All`
 * static helpers that operate over a whole map's worth of signs at once.
 *
 * @see AreaSign (the container), MapDBEntrySign (map-defined sign data).
 */
class SAVEFILE_AUTOPORT SignData : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int x MEMBER x NOTIFY xChanged)          ///< Sign tile X.
  Q_PROPERTY(int y MEMBER y NOTIFY yChanged)          ///< Sign tile Y.
  Q_PROPERTY(int txtId MEMBER txtId NOTIFY txtIdChanged) ///< Text id shown when read.

public:
  SignData(SaveFile* saveFile = nullptr, var8 index = 0);
  virtual ~SignData();

  void load(SaveFile* saveFile = nullptr, var8 index = 0); ///< Expand sign @p index from the save.
  void save(SaveFile* saveFile, var8 index);               ///< Flatten sign @p index to the save.

signals:
  void xChanged();
  void yChanged();
  void txtIdChanged();

public slots:
  void reset();                                       ///< Blank this sign.
  void randomize(QVector<TmpSignPos*>* tmpPos = nullptr); ///< Randomize position (avoiding @p tmpPos clashes).
  static QVector<SignData*> randomizeAll(QVector<MapDBEntrySign*> mapSigns); ///< Randomize a whole map's signs.

  void setTo(MapDBEntrySign* signData);               ///< Copy values from a map-defined sign.
  static QVector<SignData*> setToAll(QVector<MapDBEntrySign*> mapSigns); ///< Build signs from a map's sign list.

public:
  int x;     ///< @see x property.
  int y;     ///< @see y property.
  int txtId; ///< @see txtId property.
};
