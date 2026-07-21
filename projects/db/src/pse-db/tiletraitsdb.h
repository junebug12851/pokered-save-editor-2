/*
  * Copyright 2026 Fairy Fox
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
#include <QHash>
#include <QVector>

#include <pse-common/types.h>
#include "./db_autoport.h"

class QQmlEngine;

/**
 * @brief What a tile MEANS -- wall, grass, door, warp, ledge, shop counter.
 *
 * BlocksDB says what the map *is*; this says what it *does*. None of it is in the save
 * file: the save only carries a **pointer** (`wTilesetCollisionPtr`) at data that lives in
 * the cartridge, so the app has to carry the same data the cartridge does. Imported from
 * pret/pokered by `scripts/import_tile_traits.py` and **verified against the real
 * cartridge** -- every collision list re-read out of ROM bank 0 and compared byte-for-byte.
 *
 * ## The one thing to understand
 *
 * **There is no "is a wall" flag.** The ROM stores the list of tiles that are **PASSABLE**,
 * `$FF`-terminated, and `CheckTilePassable` walks it: a tile that is *not in the list* is a
 * wall. So "does this collide?" is derived, for every tile, from @ref passable -- it is
 * never something a user types in.
 *
 * ⚠️ And the lists are **shared** between tilesets (Red's House 1 and 2 are one list; Mart
 * and Poké Center are one list). Assuming one list per tileset, in index order, is exactly
 * how three of `tileset.json`'s `collPtr`s came to be wrong. See
 * `notes/reference/tiles.md`.
 *
 * ## Layers
 *
 * Everything here is **tile**-level (8x8) except @ref cutTreeBlocks, which is **block**-level
 * (32x32) -- the ROM swaps a whole block when you Cut a tree, not a tile. Mixing those two up
 * is the standing trap of this whole area.
 *
 * Standard DB-singleton: private constructor, `inst()`, and load() is called by DB::loadAll()
 * -- never from the constructor (Qt 6 static-init deadlock; see `decisions/architecture.md`).
 *
 * @see TilesetDB (the tileset's own header), BlocksDB (the blocks), MapEngine (the pixels),
 *      `notes/reference/tiles.md`.
 */
class DB_AUTOPORT TileTraitsDB : public QObject
{
  Q_OBJECT

public:
  static TileTraitsDB* inst(); ///< The process-wide TileTraitsDB singleton.

  /// The animated water tile. The same id in every tileset that has water at all.
  static constexpr var8 waterTile = 0x14;

  /// What one tile of one tileset does. A tile can be several of these at once.
  enum Trait : quint32 {
    None       = 0,
    Passable   = 1 << 0,  ///< In the tileset's collision list -- you can walk on it.
    Wall       = 1 << 1,  ///< NOT in the list. Derived, never stored. @see passable
    Grass      = 1 << 2,  ///< `wGrassTile`: wild encounters, and drawn over the player's feet.
    Water      = 1 << 3,  ///< @ref waterTile, in a tileset that has water.
    Warp       = 1 << 4,  ///< Step on it and you warp.
    Door       = 1 << 5,  ///< A door (a warp you walk *into*, with the door animation).
    Ledge      = 1 << 6,  ///< A ledge -- and it knows which way you jump. @see ledgeFacing
    Counter    = 1 << 7,  ///< A talk-over tile: the shop desk you can speak across.
    Bookshelf  = 1 << 8,  ///< It talks back (bookshelf, sign, statue, PC...).
    WarpPad    = 1 << 9,  ///< A Silph/gym warp pad.
    Hole       = 1 << 10, ///< A hole you fall through.
    Elevation  = 1 << 11, ///< Has a tile-pair collision: you cannot cross to some neighbour.
  };
  Q_DECLARE_FLAGS(Traits, Trait)

  /// One ledge: you may hop from @ref standingOn onto @ref tile, in direction @ref facing.
  /// (Named `LedgeRule`, not `Ledge` -- the @ref Ledge trait flag already has that name.)
  struct LedgeRule {
    QString facing;   ///< "up" / "down" / "left" / "right" -- the way you jump.
    var8 standingOn;  ///< The tile you must be standing on.
    var8 tile;        ///< The ledge tile itself.
  };

  /// Cut swaps a whole BLOCK, not a tile. (`cut_tree_blocks.asm`)
  struct CutTree {
    var8 block;  ///< The block with the tree in it.
    var8 cutTo;  ///< What it becomes.
  };

  /// One tileset's tile semantics.
  struct Entry {
    int ind = -1;
    QString name;                       ///< The ROM's constant, e.g. "REDS_HOUSE_1".
    QVector<var8> passable;             ///< The collision list. Absence from it = a wall.
    QVector<var8> warp;
    QVector<var8> door;
    QVector<var8> bookshelf;
    QVector<var8> warpPad;
    QVector<var8> hole;
    QVector<QPair<var8, var8>> pairCollisions;      ///< Land elevation edges.
    QVector<QPair<var8, var8>> pairCollisionsWater; ///< Water elevation edges.
    bool hasWater = false;
    bool isDungeon = false;             ///< Re-positions you on warp-in (`LoadTilesetHeader`).
    bool allowsBike = false;
    bool allowsEscapeRope = false;
  };

  const Entry* at(int tilesetInd) const; ///< @p tilesetInd's semantics, or nullptr.

  /**
   * @brief Everything @p tile does in @p tilesetInd, as one flag set.
   *
   * @param grassTile   the save's `wGrassTile` (`$FF` = none) -- it is a SAVE value, not a
   *                    tileset one, so the caller passes what the save actually holds rather
   *                    than what the tileset would default to. An edited save can move the
   *                    grass, and the map must show the grass where the save puts it.
   * @param counters    the save's three talk-over tiles (`$FF` = unused slot), same reasoning.
   *
   * Every tile gets exactly one of @ref Passable / @ref Wall -- that is the whole point of
   * the collision list.
   */
  Traits traitsOf(int tilesetInd, var8 tile,
                  var8 grassTile = 0xFF,
                  const QVector<var8>& counters = {}) const;

  /// The way you jump off @p tile, or an empty string if it is not a ledge.
  QString ledgeFacing(var8 tile) const;

  const QVector<LedgeRule>& ledges() const;  ///< Every ledge in the game (8 of them).
  const QVector<CutTree>& cutTreeBlocks() const; ///< Every cuttable BLOCK (9 of them).

  /// Is @p block a tree that Cut turns into something else?
  bool isCutTreeBlock(var8 block) const;

  int count() const; ///< Number of tilesets (24).

public slots:
  void load();  ///< Load tileTraits.json. Called by DB::loadAll().
  void qmlProtect(const QQmlEngine* const engine) const; ///< Pin to C++ ownership.

private slots:
  void qmlRegister() const; ///< Register with the QML type system.

private:
  TileTraitsDB(); ///< Private -- use inst().

  QHash<int, Entry> store;    ///< tilesetInd -> its semantics.
  QVector<LedgeRule> ledgeStore;
  QVector<CutTree> cutTreeStore;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TileTraitsDB::Traits)
