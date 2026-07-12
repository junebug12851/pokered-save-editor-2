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
 * @file mapmodel.cpp
 * @brief Implementation of MapModel -- the loaded map's image + geometry.
 *        See mapmodel.h.
 */

#include <pse-db/blocksdb.h>
#include <pse-db/mapsdb.h>
#include <pse-db/tileset.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-savefile/expanded/area/areamap.h>
#include <pse-savefile/expanded/area/areaplayer.h>
#include <pse-savefile/expanded/area/areatileset.h>

#include "./mapmodel.h"
#include "../engine/mapengine.h"

MapModel::MapModel(AreaMap* map, AreaPlayer* player, AreaTileset* tileset)
  : map(map), player(player), tileset(tileset)
{
  // Everything this model publishes is derived from these four values, so one
  // "changed" signal for the lot is honest -- any of them moving redraws the map.
  connect(map, &AreaMap::curMapChanged, this, &MapModel::changed);
  connect(tileset, &AreaTileset::currentChanged, this, &MapModel::changed);
  connect(player, &AreaPlayer::xCoordChanged, this, &MapModel::changed);
  connect(player, &AreaPlayer::yCoordChanged, this, &MapModel::changed);
}

int MapModel::mapInd() const     { return map->curMap; }
int MapModel::tilesetInd() const { return tileset->current; }
int MapModel::playerX() const    { return player->xCoord; }
int MapModel::playerY() const    { return player->yCoord; }

bool MapModel::valid() const
{
  return MapEngine::sourceMap(mapInd()) != nullptr
      && BlocksDB::inst()->hasTileset(tilesetInd())
      && blocksWide() > 0 && blocksHigh() > 0;
}

QString MapModel::source() const
{
  if (!valid())
    return QString();

  // Frame 0: a still map. (The flower/water animation frames are a later step.)
  return "image://map/" + QString::number(mapInd())
       + "/" + QString::number(tilesetInd())
       + "/0";
}

QString MapModel::mapName() const
{
  auto* entry = MapsDB::inst()->getIndAt(QString::number(mapInd()));
  return (entry == nullptr) ? QString() : entry->bestName();
}

bool MapModel::isCopy() const
{
  auto* source = MapEngine::sourceMap(mapInd());
  return source != nullptr && source->getInd() != mapInd();
}

QString MapModel::copyOfName() const
{
  auto* source = MapEngine::sourceMap(mapInd());

  if (source == nullptr || source->getInd() == mapInd())
    return QString();

  return source->bestName();
}

QString MapModel::tilesetName() const
{
  for (auto* el : TilesetDB::inst()->getStore())
    if (el->ind == tilesetInd())
      return el->name;

  return QString();
}

// The size comes from whichever map actually supplies the data -- a glitch id carries
// no dimensions of its own, so it takes the ones from the map it is a copy of.

int MapModel::blocksWide() const
{
  auto* entry = MapEngine::sourceMap(mapInd());
  return (entry == nullptr) ? 0 : entry->getWidth();
}

int MapModel::blocksHigh() const
{
  auto* entry = MapEngine::sourceMap(mapInd());
  return (entry == nullptr) ? 0 : entry->getHeight();
}

int MapModel::blockSize() const { return MapEngine::blockPx; }

int MapModel::imageWidth() const
{
  return (blocksWide() + 2 * MapEngine::mapBorder) * MapEngine::blockPx;
}

int MapModel::imageHeight() const
{
  return (blocksHigh() + 2 * MapEngine::mapBorder) * MapEngine::blockPx;
}

int MapModel::mapX() const { return MapEngine::mapRect(blocksWide(), blocksHigh()).x(); }
int MapModel::mapY() const { return MapEngine::mapRect(blocksWide(), blocksHigh()).y(); }
int MapModel::mapW() const { return MapEngine::mapRect(blocksWide(), blocksHigh()).width(); }
int MapModel::mapH() const { return MapEngine::mapRect(blocksWide(), blocksHigh()).height(); }

int MapModel::scratchX() const { return MapEngine::scratchRect(playerX(), playerY()).x(); }
int MapModel::scratchY() const { return MapEngine::scratchRect(playerX(), playerY()).y(); }
int MapModel::scratchW() const { return MapEngine::scratchRect(playerX(), playerY()).width(); }
int MapModel::scratchH() const { return MapEngine::scratchRect(playerX(), playerY()).height(); }

int MapModel::screenX() const { return MapEngine::screenRect(playerX(), playerY()).x(); }
int MapModel::screenY() const { return MapEngine::screenRect(playerX(), playerY()).y(); }
int MapModel::screenW() const { return MapEngine::screenRect(playerX(), playerY()).width(); }
int MapModel::screenH() const { return MapEngine::screenRect(playerX(), playerY()).height(); }
