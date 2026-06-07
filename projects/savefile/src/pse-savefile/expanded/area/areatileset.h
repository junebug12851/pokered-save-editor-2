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
struct MapDBEntry;

constexpr var8 maxTalkingOverTiles = 3; ///< Number of "talk-over" tile slots.

/**
 * @brief The current map's tileset: which set, its behaviour type, and pointers.
 *
 * Selects the active tileset and carries its associated data (grass tile, boulder
 * indices, the gameplay @ref type, and the bank/pointer locations of its graphics,
 * blocks, and collision). Several fields are "change at your own risk" -- altering
 * them can make the map unplayable, as the field comments warn. Standard
 * expanded-node convention (see SaveFileExpanded).
 *
 * @see Area, MapDBEntry.
 */
class SAVEFILE_AUTOPORT AreaTileset : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int current MEMBER current NOTIFY currentChanged)             ///< Active tileset id (risky to change).
  Q_PROPERTY(int grassTile MEMBER grassTile NOTIFY grassTileChanged)       ///< Which tile is grass.
  Q_PROPERTY(int boulderIndex MEMBER boulderIndex NOTIFY boulderIndexChanged) ///< Boulder tile index.
  Q_PROPERTY(int boulderColl MEMBER boulderColl NOTIFY boulderCollChanged) ///< Boulder collision value.
  Q_PROPERTY(int type MEMBER type NOTIFY typeChanged)                      ///< Tileset behaviour type (Outside/etc).
  Q_PROPERTY(int bank MEMBER bank NOTIFY bankChanged)                      ///< Bank holding GFX + blocks.
  Q_PROPERTY(int blockPtr MEMBER blockPtr NOTIFY blockPtrChanged)          ///< Blocks pointer.
  Q_PROPERTY(int gfxPtr MEMBER gfxPtr NOTIFY gfxPtrChanged)                ///< Graphics pointer.
  Q_PROPERTY(int collPtr MEMBER collPtr NOTIFY collPtrChanged)             ///< Collision pointer (always bank 0).

public:
  AreaTileset(SaveFile* saveFile = nullptr);
  virtual ~AreaTileset();

  void load(SaveFile* saveFile = nullptr); ///< Expand the tileset block from the save.
  void save(SaveFile* saveFile);           ///< Flatten the tileset block to the save.

  Q_INVOKABLE int talkingOverTilesCount();          ///< Number of talk-over tile slots.
  Q_INVOKABLE int talkingOverTilesAt(int ind);      ///< Talk-over tile at @p ind.
  Q_INVOKABLE void talkingOverTilesSwap(int from, int to); ///< Reorder talk-over tiles.

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
  void reset();              ///< Blank the tileset block.
  void randomize();          ///< Randomize the tileset.
  void loadFromData(MapDBEntry* map, bool randomType = false); ///< Set from @p map (optionally random type).

public:
  // Which tileset to use. Changing this will make the map
  // unplayable unless you know what your doing.
  int current;

  // Which tiles can you talk over when 1 tile sits between you and an NPC
  // Mainly used for Pokemon Centers
  var8 talkingOverTiles[maxTalkingOverTiles];

  /// Which tile is a grass tile? In testing I got odd results changing this
  int grassTile;

  // These are somewhat abstract, they're used when your adjancent to boulders
  // or when your interacting/interacted with boulders. I don't think I've seen
  // them used in a sav file
  int boulderIndex;
  int boulderColl;

  // This is something that can be freely changed, there are 3 tilesets and they
  // are treated differently on gameplay. "Outside" animates flower and water
  // tiles replacing whatever tiles were there so changing SS anne to outside
  // causes the windows to be animated flowers ~_^ but the types do other stuff
  // as well
  int type;

  // Tileset location, bank and pointers. Changing these will make the map
  // unplayable unless you know what your doing.
  // For a tileset
  // * GFX & Blocks are always on the same bank, the bank number tells you where
  // * Collision is always in bank 0 (Home Bank)
  int bank;
  int blockPtr;
  int gfxPtr;
  int collPtr;
};
