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
class SpriteData;
class MapDBEntry;
class MapDBEntrySprite;

constexpr var8 maxSprites = 16; ///< Maximum sprites/NPCs on a map.

/**
 * @brief The current map's list of sprites/NPCs.
 *
 * A variable-length list of SpriteData (up to @ref maxSprites) with QML add/remove/
 * swap/access. setTo()/randomize() rebuild it from a chosen map's sprite set.
 * Standard expanded-node convention (see SaveFileExpanded).
 *
 * @see SpriteData (a sprite), Area, MapDBEntrySprite.
 */
class SAVEFILE_AUTOPORT AreaSprites : public QObject
{
  Q_OBJECT

public:
  AreaSprites(SaveFile* saveFile = nullptr);
  virtual ~AreaSprites();

  void load(SaveFile* saveFile = nullptr); ///< Expand the sprite list from the save.
  void save(SaveFile* saveFile);           ///< Flatten the sprite list to the save.

  Q_INVOKABLE int spriteCount();             ///< Number of sprites.
  Q_INVOKABLE int spriteMax();               ///< Capacity (maxSprites).
  Q_INVOKABLE SpriteData* spriteAt(int ind); ///< Sprite @p ind (GC-protected return).
  Q_INVOKABLE void spriteSwap(int from, int to); ///< Reorder sprites.
  Q_INVOKABLE void spriteRemove(int ind);    ///< Remove sprite @p ind (the rest slide up).
  Q_INVOKABLE void spriteNew();              ///< Add a fresh sprite.

  /// Add a sprite of picture @p pictureID at map (@p x, @p y) -- the drag-from-the-
  /// Characters-bar path. Returns the new sprite's slot, or -1 if the map is full.
  Q_INVOKABLE int spriteAdd(int pictureID, int x, int y);

  /// Move sprite @p ind to map (@p x, @p y). Writes **exactly two bytes** (mapX, mapY),
  /// carrying the game's +4 bias. The drag-on-the-canvas path.
  Q_INVOKABLE void spriteMove(int ind, int x, int y);

  /**
   * @note **A sprite's visibility does not live here.** A "missable" sprite is hidden by a
   * global flag in `wToggleableObjectFlags` (save `0x2852`) -- *bit set = toggled OFF* --
   * which SpriteData::missableIndex points at, and which @ref WorldMissables owns. The
   * per-map list at `0x287A` only maps a slot to a flag index, and the game rebuilds it from
   * ROM on every map load. See notes/reference/sprites.md -> Part 3.
   */

signals:
  void spritesChanged();

public slots:
  void reset();                                        ///< Empty the sprite list.
  void randomize(QVector<MapDBEntrySprite*> spriteData); ///< Randomize sprites from @p spriteData.
  void setTo(MapDBEntry* map);                         ///< Rebuild the list from @p map's sprites.

public:
  QVector<SpriteData*> sprites; ///< The map's sprites/NPCs.
};
