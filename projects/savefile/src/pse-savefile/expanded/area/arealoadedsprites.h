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
struct MapDBEntry;
struct SpriteSetDBEntry;

constexpr var8 maxLoadedSprites = 11; ///< Fixed number of sprite slots the game keeps loaded.

/**
 * @brief The fixed set of sprite picture-ids currently loaded into the map's VRAM.
 *
 * The game keeps exactly @ref maxLoadedSprites loaded sprite entries plus the
 * @ref loadedSetId of the chosen sprite set. These slots are fixed -- they can be
 * swapped but not created/removed -- so the QML helpers only count, read, and swap.
 * Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see Area, SpriteSetDBEntry.
 */
class SAVEFILE_AUTOPORT AreaLoadedSprites : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int loadedSetId MEMBER loadedSetId NOTIFY loadedSetIdChanged) ///< Which sprite set is loaded.

public:
  AreaLoadedSprites(SaveFile* saveFile = nullptr);
  virtual ~AreaLoadedSprites();

  void load(SaveFile* saveFile = nullptr); ///< Expand the loaded-sprite slots from the save.
  void save(SaveFile* saveFile);           ///< Flatten the loaded-sprite slots to the save.
  void loadSpriteSet(SpriteSetDBEntry* entry, int x, int y); ///< Populate from a sprite-set definition.

  // Loaded sprites are a fixed size and cannot be moved, created, modified, or destroyed
  // They can be swapped
  Q_INVOKABLE int lSpriteCount();              ///< Number of loaded-sprite slots (fixed).
  Q_INVOKABLE int lSpriteAt(int ind);          ///< Picture-id in slot @p ind.
  Q_INVOKABLE void lSpriteSet(int ind, int picture); ///< Put @p picture in slot @p ind. One byte.
  Q_INVOKABLE void lSpriteSwap(int from, int to); ///< Swap two loaded-sprite slots.

signals:
  void loadedSpritesChanged();
  void loadedSetIdChanged();

public slots:
  void reset();                                  ///< Blank the loaded-sprite slots.
  void randomize(MapDBEntry* map, int x, int y); ///< Randomize for @p map at (x,y).
  void setTo(MapDBEntry* map, int x, int y);     ///< Set to @p map's sprite set at (x,y).

public:
  var8 loadedSprites[maxLoadedSprites]; ///< The fixed loaded-sprite picture-ids.
  int loadedSetId;                      ///< @see loadedSetId property.
};
