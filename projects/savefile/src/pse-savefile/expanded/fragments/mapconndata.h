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
#include <pse-common/types.h>
#include "../../savefile_autoport.h"

class SaveFile;
struct MapDBEntry;
struct MapDBEntryConnect;

/**
 * @brief One edge connection of the current map (the seam to a neighbouring map).
 *
 * Gen 1 maps stitch together at their edges; each connection records the
 * neighbouring map pointer, the source/destination strips, width, and alignment
 * offsets, plus the view pointer. These are low-level layout values mirrored from
 * the game's connection block. Can be populated from the save or from a map DB
 * connection (loadFromData).
 *
 * @see AreaMap / Area (the map state), MapDBEntryConnect (map-defined connection).
 */
class SAVEFILE_AUTOPORT MapConnData : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int mapPtr MEMBER mapPtr NOTIFY mapPtrChanged)             ///< Pointer to the connected map's data.
  Q_PROPERTY(int stripSrc MEMBER stripSrc NOTIFY stripSrcChanged)       ///< Source strip pointer.
  Q_PROPERTY(int stripDst MEMBER stripDst NOTIFY stripDstChanged)       ///< Destination strip pointer.
  Q_PROPERTY(int stripWidth MEMBER stripWidth NOTIFY stripWidthChanged) ///< Strip width.
  Q_PROPERTY(int width MEMBER width NOTIFY widthChanged)                ///< Connected map width.
  Q_PROPERTY(int yAlign MEMBER yAlign NOTIFY yAlignChanged)             ///< Vertical alignment offset.
  Q_PROPERTY(int xAlign MEMBER xAlign NOTIFY xAlignChanged)             ///< Horizontal alignment offset.
  Q_PROPERTY(int viewPtr MEMBER viewPtr NOTIFY viewPtrChanged)          ///< View pointer.

public:
  MapConnData(SaveFile* saveFile = nullptr, var16 offset = 0);
  virtual ~MapConnData();

  void load(SaveFile* saveFile = nullptr, var16 offset = 0); ///< Expand a connection block from the save.
  void save(SaveFile* saveFile, var16 offset);               ///< Flatten a connection block to the save.
  void loadFromData(MapDBEntryConnect* connect);             ///< Populate from a map-defined connection.

  MapDBEntry* toMap(); ///< The connected map's DB entry.

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
  void reset(); ///< Blank this connection.

public:
  int mapPtr;     ///< @see mapPtr property.
  int stripSrc;   ///< @see stripSrc property.
  int stripDst;   ///< @see stripDst property.
  int stripWidth; ///< @see stripWidth property.
  int width;      ///< @see width property.
  int yAlign;     ///< @see yAlign property.
  int xAlign;     ///< @see xAlign property.
  int viewPtr;    ///< @see viewPtr property.
};
