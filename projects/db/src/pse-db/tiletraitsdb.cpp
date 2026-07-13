/*
  * Copyright 2026 Twilight
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

/**
 * @file tiletraitsdb.cpp
 * @brief Implementation of TileTraitsDB -- what every tile of every tileset means.
 *        See tiletraitsdb.h and notes/reference/tiles.md.
 */

#include <QJsonArray>
#include <QJsonObject>
#include <QQmlEngine>

#include <pse-common/utility.h>
#include "./tiletraitsdb.h"
#include "./util/gamedata.h"

namespace {
/// An unused slot / "there is no such tile". The ROM writes -1 into these fields.
constexpr var8 noTile = 0xFF;

QVector<var8> readTiles(const QJsonObject& obj, const QString& key)
{
  QVector<var8> out;
  for (const QJsonValue v : obj.value(key).toArray())
    out.append(static_cast<var8>(v.toInt()));
  return out;
}

QVector<QPair<var8, var8>> readPairs(const QJsonObject& obj, const QString& key)
{
  QVector<QPair<var8, var8>> out;
  for (const QJsonValue v : obj.value(key).toArray()) {
    const QJsonArray pair = v.toArray();
    if (pair.size() == 2)
      out.append({ static_cast<var8>(pair.at(0).toInt()),
                   static_cast<var8>(pair.at(1).toInt()) });
  }
  return out;
}
} // namespace

TileTraitsDB* TileTraitsDB::inst()
{
  static TileTraitsDB* _inst = new TileTraitsDB;
  return _inst;
}

// Deliberately does NOT load -- DB::loadAll() does that. Loading in a DB constructor
// deadlocks Qt 6 static init (decisions/architecture.md).
TileTraitsDB::TileTraitsDB()
{
  qmlRegister();
}

void TileTraitsDB::load()
{
  static bool once = false;
  if (once)
    return;

  const QJsonObject root = GameData::inst()->json("tileTraits").object();

  for (const QJsonValue v : root.value("tilesets").toArray()) {
    const QJsonObject obj = v.toObject();

    Entry e;
    e.ind = obj.value("ind").toInt(-1);
    e.name = obj.value("name").toString();
    e.passable = readTiles(obj, "passable");
    e.warp = readTiles(obj, "warp");
    e.door = readTiles(obj, "door");
    e.bookshelf = readTiles(obj, "bookshelf");
    e.pairCollisions = readPairs(obj, "pairCollisions");
    e.pairCollisionsWater = readPairs(obj, "pairCollisionsWater");
    e.hasWater = obj.value("hasWater").toBool();
    e.isDungeon = obj.value("isDungeon").toBool();
    e.allowsBike = obj.value("allowsBike").toBool();
    e.allowsEscapeRope = obj.value("allowsEscapeRope").toBool();

    // A pad and a hole are the same table with a different verb, so they arrive together
    // and get split here -- one drops you somewhere, the other drops you through.
    for (const QJsonValue pv : obj.value("warpPadHole").toArray()) {
      const QJsonObject po = pv.toObject();
      const var8 tile = static_cast<var8>(po.value("tile").toInt());
      if (po.value("kind").toString() == "pad")
        e.warpPad.append(tile);
      else
        e.hole.append(tile);
    }

    if (e.ind >= 0)
      store.insert(e.ind, e);
  }

  for (const QJsonValue v : root.value("ledges").toArray()) {
    const QJsonObject obj = v.toObject();
    ledgeStore.append({ obj.value("facing").toString(),
                        static_cast<var8>(obj.value("standingOn").toInt()),
                        static_cast<var8>(obj.value("tile").toInt()) });
  }

  for (const QJsonValue v : root.value("cutTreeBlocks").toArray()) {
    const QJsonObject obj = v.toObject();
    cutTreeStore.append({ static_cast<var8>(obj.value("block").toInt()),
                          static_cast<var8>(obj.value("cutTo").toInt()) });
  }

  once = true;
}

const TileTraitsDB::Entry* TileTraitsDB::at(int tilesetInd) const
{
  const auto it = store.constFind(tilesetInd);
  return (it == store.constEnd()) ? nullptr : &it.value();
}

TileTraitsDB::Traits TileTraitsDB::traitsOf(int tilesetInd, var8 tile,
                                            var8 grassTile,
                                            const QVector<var8>& counters) const
{
  const Entry* e = at(tilesetInd);
  if (e == nullptr)
    return Traits(None);

  Traits t = None;

  // THE line. The ROM stores what you CAN walk on; everything else is a wall. There is no
  // "wall" list to read -- it is the complement, and this is where that gets decided.
  t |= e->passable.contains(tile) ? Passable : Wall;

  if (e->warp.contains(tile))       t |= Warp;
  if (e->door.contains(tile))       t |= Door;
  if (e->bookshelf.contains(tile))  t |= Bookshelf;
  if (e->warpPad.contains(tile))    t |= WarpPad;
  if (e->hole.contains(tile))       t |= Hole;

  // Water is one id, but only in a tileset that has water at all -- in every other
  // tileset $14 is just some floor.
  if (e->hasWater && tile == waterTile)
    t |= Water;

  // Grass and the counters come from the SAVE, not the tileset: an edited save can put the
  // grass somewhere else, and the map must show it where the save actually puts it.
  if (grassTile != noTile && tile == grassTile)
    t |= Grass;

  for (const var8 c : counters) {
    if (c != noTile && c == tile) {
      t |= Counter;
      break;
    }
  }

  if (!ledgeFacing(tile).isEmpty())
    t |= Ledge;

  for (const auto& pair : e->pairCollisions) {
    if (pair.first == tile || pair.second == tile) {
      t |= Elevation;
      break;
    }
  }
  if (!(t & Elevation)) {
    for (const auto& pair : e->pairCollisionsWater) {
      if (pair.first == tile || pair.second == tile) {
        t |= Elevation;
        break;
      }
    }
  }

  return t;
}

QString TileTraitsDB::ledgeFacing(var8 tile) const
{
  for (const LedgeRule& l : ledgeStore) {
    if (l.tile == tile)
      return l.facing;
  }
  return QString();
}

const QVector<TileTraitsDB::LedgeRule>& TileTraitsDB::ledges() const
{
  return ledgeStore;
}

const QVector<TileTraitsDB::CutTree>& TileTraitsDB::cutTreeBlocks() const
{
  return cutTreeStore;
}

bool TileTraitsDB::isCutTreeBlock(var8 block) const
{
  for (const CutTree& t : cutTreeStore) {
    if (t.block == block)
      return true;
  }
  return false;
}

int TileTraitsDB::count() const
{
  return store.size();
}

void TileTraitsDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void TileTraitsDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<TileTraitsDB>("PSE.DB.TileTraitsDB", 1, 0, "TileTraitsDB",
                                           "Can't instantiate in QML");
  once = true;
}
