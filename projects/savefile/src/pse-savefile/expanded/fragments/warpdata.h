/*
  * Copyright 2020 June Hanabi
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
#ifndef WARPDATA_H
#define WARPDATA_H

#include <QObject>
#include <QVector>
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
class MapDBEntry;
class MapDBEntryWarpOut;

class SAVEFILE_AUTOPORT WarpData : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int y MEMBER y NOTIFY yChanged)
  Q_PROPERTY(int x MEMBER x NOTIFY xChanged)
  Q_PROPERTY(int destWarp MEMBER destWarp NOTIFY destWarpChanged)
  Q_PROPERTY(int destMap MEMBER destMap NOTIFY destMapChanged)

public:
  WarpData(SaveFile* saveFile = nullptr, var8 index = 0);
  WarpData(MapDBEntryWarpOut* warp);
  virtual ~WarpData();

  void load(SaveFile* saveFile = nullptr, var8 index = 0);
  void load(MapDBEntryWarpOut* warp);
  void save(SaveFile* saveFile, var8 index);

  MapDBEntry* toMap();

signals:
  void yChanged();
  void xChanged();
  void destWarpChanged();
  void destMapChanged();

public slots:
  void reset();
  void randomize();

public:
  int y;
  int x;
  int destWarp;
  int destMap;
};

#endif // WARPDATA_H
