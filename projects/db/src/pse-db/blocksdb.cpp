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

/**
 * @file blocksdb.cpp
 * @brief Implementation of BlocksDB -- the raw .blk / .bst block data.
 *        See blocksdb.h.
 */

#include <QFile>
#include <QQmlEngine>
#include <QVector>
#include <pse-common/utility.h>

#include "./blocksdb.h"
#include "./mapsdb.h"
#include "./entries/mapdbentry.h"

namespace {
/// Ids are one byte in the save, so this covers every map / tileset that can exist.
constexpr int maxInd = 256;

/// Read a whole resource file, or an empty array if it isn't there.
QByteArray readRes(const QString& path)
{
  QFile f(path);
  if (!f.exists() || !f.open(QIODevice::ReadOnly))
    return QByteArray();

  const QByteArray data = f.readAll();
  f.close();
  return data;
}
} // namespace

BlocksDB* BlocksDB::inst()
{
  static BlocksDB* _inst = new BlocksDB;
  return _inst;
}

// Deliberately empty -- load() is called by DB::loadAll(), never from here.
// (Loading in a DB constructor deadlocks Qt 6 static init; see decisions/architecture.md.)
BlocksDB::BlocksDB() {}

void BlocksDB::load()
{
  static bool once = false;
  if (once)
    return;

  maps.clear();
  tilesets.clear();

  // Not every id has data: the glitch maps past the last real one don't exist in
  // ROM at all, so they simply have no file and stay absent from the hash.
  for (int i = 0; i < maxInd; i++) {
    const QByteArray blk = readRes(":/assets/blocks/maps/" + QString::number(i) + ".blk");
    if (!blk.isEmpty())
      maps.insert(i, blk);

    const QByteArray bst = readRes(":/assets/blocks/tilesets/" + QString::number(i) + ".bst");
    if (!bst.isEmpty())
      tilesets.insert(i, bst);
  }

  indexRom();

  once = true;
}

void BlocksDB::indexRom()
{
  // Put every map's blocks back where they actually live in the cartridge: its own bank,
  // at its own dataPtr (the address of its `_Blocks` label). That is what lets a
  // connection's ConnectionStripSrc be read as a POINTER -- the way the game reads it --
  // rather than as an index into a per-map array it isn't.
  rom.clear();

  for (auto* entry : MapsDB::inst()->getStore()) {
    const QByteArray blocks = mapBlocks(entry->getInd());
    if (blocks.isEmpty() || entry->getBank() < 0 || entry->getDataPtr() <= 0)
      continue;

    rom[entry->getBank()].append({ entry->getDataPtr(), blocks });
  }
}

int BlocksDB::blockAt(int bank, int addr) const
{
  for (const Region& region : rom.value(bank)) {
    const int i = addr - region.addr;
    if (i >= 0 && i < region.blocks.size())
      return static_cast<quint8>(region.blocks[i]);
  }

  // ROM we don't hold (headers, scripts, text -- we don't ship the cartridge). Unknown,
  // and we say so rather than invent a block.
  return -1;
}

QByteArray BlocksDB::mapBlocks(int mapInd) const
{
  return maps.value(mapInd, QByteArray());
}

QByteArray BlocksDB::tilesetBlocks(int tilesetInd) const
{
  return tilesets.value(tilesetInd, QByteArray());
}

bool BlocksDB::hasMap(int mapInd) const
{
  return maps.contains(mapInd);
}

bool BlocksDB::hasTileset(int tilesetInd) const
{
  return tilesets.contains(tilesetInd);
}

int BlocksDB::tilesetBlockCount(int tilesetInd) const
{
  return tilesets.value(tilesetInd, QByteArray()).size() / tilesPerBlock;
}

int BlocksDB::mapCount() const
{
  return maps.size();
}

int BlocksDB::tilesetCount() const
{
  return tilesets.size();
}

void BlocksDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void BlocksDB::qmlRegister() const
{
  static bool registered = false;
  if (registered)
    return;

  qmlRegisterUncreatableType<BlocksDB>("PSE.DB.BlocksDB", 1, 0, "BlocksDB", "Can't instantiate in QML");
  registered = true;
}
