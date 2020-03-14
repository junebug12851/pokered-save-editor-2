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
#include <pse-common/types.h>

class SaveFile;
struct MapDBEntry;
struct MapDBEntryConnect;

class MapConnData : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int mapPtr MEMBER mapPtr NOTIFY mapPtrChanged)
  Q_PROPERTY(int stripSrc MEMBER stripSrc NOTIFY stripSrcChanged)
  Q_PROPERTY(int stripDst MEMBER stripDst NOTIFY stripDstChanged)
  Q_PROPERTY(int stripWidth MEMBER stripWidth NOTIFY stripWidthChanged)
  Q_PROPERTY(int width MEMBER width NOTIFY widthChanged)
  Q_PROPERTY(int yAlign MEMBER yAlign NOTIFY yAlignChanged)
  Q_PROPERTY(int xAlign MEMBER xAlign NOTIFY xAlignChanged)
  Q_PROPERTY(int viewPtr MEMBER viewPtr NOTIFY viewPtrChanged)

public:
  MapConnData(SaveFile* saveFile = nullptr, var16 offset = 0);
  virtual ~MapConnData();

  void load(SaveFile* saveFile = nullptr, var16 offset = 0);
  void save(SaveFile* saveFile, var16 offset);
  void loadFromData(MapDBEntryConnect* connect);

  MapDBEntry* toMap();

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
  void reset();

public:
  int mapPtr;
  int stripSrc;
  int stripDst;
  int stripWidth;
  int width;
  int yAlign;
  int xAlign;
  int viewPtr;
};

#endif // MAPCONNDATA_H
