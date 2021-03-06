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

#include <QDebug>
#include <QQmlEngine>

#include <pse-common/random.h>
#include <pse-common/utility.h>

#include "./mapsearch.h"
#include "../mapsdb.h"
#include "../tileset.h"
#include "../spriteSet.h"

MapSearch::MapSearch()
{
  qmlRegister();
  startOver();
}

MapDBEntry* MapSearch::pickRandom()
{
  if(results.size() == 0)
    return nullptr;

  return results.at(Random::inst()->rangeExclusive(0, results.size()));
}

MapSearch* MapSearch::startOver()
{
  results.clear();

  // Copy elements over to begin search
  for(auto entry : MapsDB::store)
  {
    results.append(entry);
  }

  return this;
}

MapSearch* MapSearch::notNamed(QString val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->name == val ||
       entry->modernName == val)
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::indexLt(int val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!(entry->ind < val))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::indexGt(int val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!(entry->ind > val))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::widthGt(int val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->width || !(*entry->width > val))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::widthLt(int val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->height || !(*entry->height < val))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::heightGt(int val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->width || !(*entry->width > val))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::heightLt(int val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->width || !(*entry->width < val))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::areaGt(int val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->width || !entry->height ||
       !((*entry->width * *entry->height) > val))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::areaLt(int val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->width || !entry->height ||
       !((*entry->width * *entry->height) < val))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::hasTileset(QString val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->toTileset == nullptr ||
       (!(entry->toTileset->name == val) ||
       !(entry->toTileset->nameAlias == val)))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::notTileset(QString val)
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->toTileset == nullptr ||
       (!(entry->toTileset->name != val) &&
       !(entry->toTileset->nameAlias != val)))
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::isType(QString val)
{
  for(auto entry : QVector<MapDBEntry*>(results)) {
    if(entry->toTileset == nullptr)
      results.removeOne(entry);

    if(entry->toTileset->type != val && entry->toTileset->typeAlias != val)
      results.removeOne(entry);
  }

  return this;
}

MapSearch* MapSearch::notType(QString val)
{
  for(auto entry : QVector<MapDBEntry*>(results)) {
    if(entry->toTileset == nullptr)
      continue;

    if(entry->toTileset->type == val || entry->toTileset->typeAlias == val)
      results.removeOne(entry);
  }

  return this;
}

MapSearch* MapSearch::hasConnections()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->connect.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::noConnections()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->connect.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::hasWarpsOut()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->warpOut.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::noWarpsOut()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->warpOut.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::hasWarpsIn()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->warpIn.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::noWarpsIn()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->warpIn.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::hasSigns()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->signs.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::noSigns()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->signs.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::hasSprites()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->sprites.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::noSprites()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->sprites.isEmpty())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::hasSpriteSet()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->spriteSet)
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::noSpriteSet()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->spriteSet)
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::hasDynamicSpriteSet()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->spriteSet || !(*entry->toSpriteSet).isDynamic())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::noDynamicSpriteSet()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->spriteSet || (*entry->toSpriteSet).isDynamic())
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::hasMons()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->monRate || *entry->monRate == 0)
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::noMons()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->monRate || *entry->monRate > 0)
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::isIncomplete()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->incomplete == "")
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::notIncomplete()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->incomplete != "")
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::isGlitch()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->glitch)
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::notGlitch()
{
  for(auto entry : QVector<MapDBEntry*>(results)) {
    if(entry->glitch)
      results.removeOne(entry);
  }

  return this;
}

MapSearch* MapSearch::isSpsecial()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(!entry->special)
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::notSpecial()
{
  for(auto entry : QVector<MapDBEntry*>(results))
    if(entry->special)
      results.removeOne(entry);

  return this;
}

MapSearch* MapSearch::isGood()
{
  return notGlitch()->notSpecial()->notIncomplete()->
      hasWarpsIn()->hasWarpsOut()->notNamed("Silph Co Elevator");
}

MapSearch* MapSearch::isCity()
{
  return indexLt(11);
}

MapSearch* MapSearch::notCity()
{
  return indexGt(10);
}

const QVector<MapDBEntry*> MapSearch::getMaps() const
{
  return results;
}

int MapSearch::getMapCount() const
{
  return results.size();
}

const MapDBEntry* MapSearch::mapAt(const int ind) const
{
  if(ind >= results.size())
    return nullptr;

  return results.at(ind);
}

void MapSearch::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void MapSearch::qmlRegister() const
{
  static bool registered = false;
  if(registered)
    return;

  qmlRegisterUncreatableType<MapSearch>("PSE.DB.MapSearch", 1, 0, "MapSearch", "Can't instantiate in QML");
  registered = true;
}
