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
#ifndef AREATILESET_H
#define AREATILESET_H

#include <QObject>
#include "../../../../common/types.h"

class SaveFile;
struct MapDBEntry;

class AreaTileset : public QObject
{
  Q_OBJECT

  Q_PROPERTY(var8 current_ MEMBER current NOTIFY currentChanged)
  Q_PROPERTY(var8 grassTile_ MEMBER grassTile NOTIFY grassTileChanged)
  Q_PROPERTY(var8 boulderIndex_ MEMBER boulderIndex NOTIFY boulderIndexChanged)
  Q_PROPERTY(var8 boulderColl_ MEMBER boulderColl NOTIFY boulderCollChanged)
  Q_PROPERTY(var8 type_ MEMBER type NOTIFY typeChanged)
  Q_PROPERTY(var8 bank_ MEMBER bank NOTIFY bankChanged)
  Q_PROPERTY(var16 blockPtr_ MEMBER blockPtr NOTIFY blockPtrChanged)
  Q_PROPERTY(var16 gfxPtr_ MEMBER gfxPtr NOTIFY gfxPtrChanged)
  Q_PROPERTY(var16 collPtr_ MEMBER collPtr NOTIFY collPtrChanged)

public:
  AreaTileset(SaveFile* saveFile = nullptr);
  virtual ~AreaTileset();

  // Sort of a work around for Qt Quick and C++ Array
  // Can't be a Q_PROPERTY because we need an index argument
  Q_INVOKABLE var8 getTalkingOverTile(var8 ind);
  Q_INVOKABLE void setTalkingOverTile(var8 ind, var8 val);

signals:
  void currentChanged();
  void talkingOverTilesChanged();
  void grassTileChanged();
  void boulderIndexChanged();
  void boulderCollChanged();
  void typeChanged();
  void bankChanged();
  void blockPtrChanged();
  void gfxPtrChanged();
  void collPtrChanged();

public slots:
  void load(SaveFile* saveFile = nullptr);
  void save(SaveFile* saveFile);
  void reset();
  void randomize();
  void loadFromData(MapDBEntry* map, bool randomType = false);

public:
  // Which tileset to use. Changing this will make the map
  // unplayable unless you know what your doing.
  var8 current;

  // Which tiles can you talk over when 1 tile sits between you and an NPC
  // Mainly used for Pokemon Centers
  var8 talkingOverTiles[3];

  // Which tile is a grass tile? In testing I got odd results changing this
  var8 grassTile;

  // These are somewhat abstract, they're used when your adjancent to boulders
  // or when your interacting/interacted with boulders. I don't think I've seen
  // them used in a sav file
  var8 boulderIndex;
  var8 boulderColl;

  // This is something that can be freely changed, there are 3 tilesets and they
  // are treated differently on gameplay. "Outside" animates flower and water
  // tiles replacing whatever tiles were there so changing SS anne to outside
  // causes the windows to be animated flowers ~_^ but the types do other stuff
  // as well
  var8 type;

  // Tileset location, bank and pointers. Changing these will make the map
  // unplayable unless you know what your doing.
  // For a tileset
  // * GFX & Blocks are always on the same bank, the bank number tells you where
  // * Collision is always in bank 0 (Home Bank)
  var8 bank;
  var16 blockPtr;
  var16 gfxPtr;
  var16 collPtr;
};

#endif // AREATILESET_H
