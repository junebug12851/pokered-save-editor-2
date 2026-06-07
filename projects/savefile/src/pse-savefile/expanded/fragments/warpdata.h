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
class MapDBEntry;
class MapDBEntryWarpOut;

/**
 * @brief One warp point on the current map: its tile and where it leads.
 *
 * Holds the warp's @ref x / @ref y tile position and its destination
 * (@ref destMap + @ref destWarp index on that map). Can be built from the save or
 * from a map DB warp definition. toMap() resolves the destination to its map entry.
 *
 * @see AreaWarps (the container), MapDBEntryWarpOut (map-defined warp).
 */
class SAVEFILE_AUTOPORT WarpData : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int y MEMBER y NOTIFY yChanged)                 ///< Warp tile Y.
  Q_PROPERTY(int x MEMBER x NOTIFY xChanged)                 ///< Warp tile X.
  Q_PROPERTY(int destWarp MEMBER destWarp NOTIFY destWarpChanged) ///< Destination warp index on the target map.
  Q_PROPERTY(int destMap MEMBER destMap NOTIFY destMapChanged)    ///< Destination map id.

public:
  WarpData(SaveFile* saveFile = nullptr, var8 index = 0); ///< From the save at warp @p index.
  WarpData(MapDBEntryWarpOut* warp);                      ///< From a map-defined warp.
  virtual ~WarpData();

  void load(SaveFile* saveFile = nullptr, var8 index = 0); ///< Expand warp @p index from the save.
  void load(MapDBEntryWarpOut* warp);                      ///< Populate from a map-defined warp.
  void save(SaveFile* saveFile, var8 index);               ///< Flatten warp @p index to the save.

  MapDBEntry* toMap(); ///< The destination map's DB entry.

signals:
  void yChanged();
  void xChanged();
  void destWarpChanged();
  void destMapChanged();

public slots:
  void reset();     ///< Blank this warp.
  void randomize(); ///< Randomize this warp.

public:
  int y;        ///< @see y property.
  int x;        ///< @see x property.
  int destWarp; ///< @see destWarp property.
  int destMap;  ///< @see destMap property.
};
