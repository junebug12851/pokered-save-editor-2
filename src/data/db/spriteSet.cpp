/*
  * Copyright 2019 June Hanabi
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

#include <QJsonValue>
#include <QJsonArray>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./spriteSet.h"
#include "./gamedata.h"
#include "./sprites.h"

bool SpriteSetDBEntry::isDynamic()
{
  return ind >= 0xF1;
}

QVector<SpriteDBEntry*> SpriteSetDBEntry::getSprites(var8 x, var8 y)
{
  // If static then simply return the sprite list
  if(!isDynamic())
    return toSprites;

  // Otherwise figure out which sprite list to return
  if(split == "horz") {
    if(y < splitAt)
      return toSetWN->toSprites;
    else
      return toSetES->toSprites;
  }
  else {
    if(x < splitAt)
      return toSetWN->toSprites;
    else
      return toSetES->toSprites;
  }
}

void SpriteSetDB::load()
{
  // Grab Music Data
  auto spriteSetData = GameData::json("spriteSet");

  // Go through each music
  for(QJsonValue spriteSetEntry : spriteSetData->array())
  {
    // Create a new event Pokemon entry
    auto entry = new SpriteSetDBEntry();

    // Set simple properties
    entry->ind = spriteSetEntry["ind"].toDouble();

    if(spriteSetEntry["split"].isString())
      entry->split = spriteSetEntry["split"].toString();

    if(spriteSetEntry["splitAt"].isString())
      entry->splitAt = spriteSetEntry["splitAt"].toDouble();

    if(spriteSetEntry["setWN"].isDouble())
      entry->setWN = spriteSetEntry["setWN"].toDouble();

    if(spriteSetEntry["setES"].isDouble())
      entry->setES = spriteSetEntry["setES"].toDouble();

    if(spriteSetEntry["sprites"].isArray()) {
      for(QJsonValue spriteListEntry : spriteSetEntry["sprites"].toArray())
        entry->spriteList.append(spriteListEntry.toString());
    }

    // Add to array
    store.append(entry);
  }

  delete spriteSetData;
}

void SpriteSetDB::index()
{
  for(auto entry : store)
  {
    // Index name and index
    ind.insert(QString::number(entry->ind), entry);
  }
}

void SpriteSetDB::deepLink()
{
  for(auto entry : store)
  {
    for(auto spriteEntry : entry->spriteList)
    {
      auto tmp = SpritesDB::ind.value(spriteEntry, nullptr);
      entry->toSprites.append(tmp);

#ifdef QT_DEBUG
    if(tmp == nullptr)
      qCritical() << "SpriteSetDB: " << spriteEntry << ", could not be deep linked to sprite." ;
#endif
    }

    if(entry->setWN) {
      entry->toSetWN = SpriteSetDB::ind.value(QString::number(*entry->setWN), nullptr);

#ifdef QT_DEBUG
    if(entry->setWN && entry->toSetWN == nullptr)
      qCritical() << "SpriteSetDB Set WN: " << *entry->setWN << ", could not be deep linked to static set." ;
#endif
    }

    if(entry->setES) {
      entry->toSetES = SpriteSetDB::ind.value(QString::number(*entry->setES), nullptr);

#ifdef QT_DEBUG
    if(entry->setES && entry->toSetES == nullptr)
      qCritical() << "SpriteSetDB Set ES: " << *entry->setES << ", could not be deep linked to static set." ;
#endif
    }
  }
}

QVector<SpriteSetDBEntry*> SpriteSetDB::store = QVector<SpriteSetDBEntry*>();
QHash<QString, SpriteSetDBEntry*> SpriteSetDB::ind =
    QHash<QString, SpriteSetDBEntry*>();
