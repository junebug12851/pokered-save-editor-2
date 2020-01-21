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
#ifndef MAPCONNDATA_H
#define MAPCONNDATA_H

#include <QObject>
#include "../../../../common/types.h"

class SaveFile;
struct MapDBEntry;
struct MapDBEntryConnect;

class MapConnData : public QObject
{
  Q_OBJECT

  Q_PROPERTY(var8 mapPtr_ MEMBER mapPtr NOTIFY mapPtrChanged)
  Q_PROPERTY(var16 stripSrc_ MEMBER stripSrc NOTIFY stripSrcChanged)
  Q_PROPERTY(var16 stripDst_ MEMBER stripDst NOTIFY stripDstChanged)
  Q_PROPERTY(var8 stripWidth_ MEMBER stripWidth NOTIFY stripWidthChanged)
  Q_PROPERTY(var8 width_ MEMBER width NOTIFY widthChanged)
  Q_PROPERTY(var8 yAlign_ MEMBER yAlign NOTIFY yAlignChanged)
  Q_PROPERTY(var8 xAlign_ MEMBER xAlign NOTIFY xAlignChanged)
  Q_PROPERTY(var16 viewPtr_ MEMBER viewPtr NOTIFY viewPtrChanged)

public:
  MapConnData(SaveFile* saveFile = nullptr, var16 offset = 0);
  virtual ~MapConnData();

  Q_INVOKABLE MapDBEntry* toMap();

signals:
  void mapPtrChanged();
  void stripSrcChanged();
  void stripDstChanged();
  void stripWidthChanged();
  void widthChanged();
  void yAlignChanged();
  void xAlignChanged();
  void viewPtrChanged();

public slots:
  void load(SaveFile* saveFile = nullptr, var16 offset = 0);
  void save(SaveFile* saveFile, var16 offset);
  void reset();
  void loadFromData(MapDBEntryConnect* connect);

public:
  var8 mapPtr;
  var16 stripSrc;
  var16 stripDst;
  var8 stripWidth;
  var8 width;
  var8 yAlign;
  var8 xAlign;
  var16 viewPtr;
};

#endif // MAPCONNDATA_H
