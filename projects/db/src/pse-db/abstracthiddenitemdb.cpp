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

#include <pse-common/utility.h>
#include <QVector>
#include <QJsonArray>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./abstracthiddenitemdb.h"
#include "./maps.h"
#include "./util/gamedata.h"
#include "./entries/hiddenitemdbentry.h"

const QVector<HiddenItemDBEntry*> AbstractHiddenItemDB::getStore() const
{
  return store;
}

int AbstractHiddenItemDB::getStoreSize() const
{
  return store.size();
}

const HiddenItemDBEntry* AbstractHiddenItemDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

void AbstractHiddenItemDB::load()
{
  static bool once = false;
  if(once)
    return;

  // Grab Event Pokemon Data
  auto jsonData = GameData::inst()->json(loadFile);

  // Go through each event Pokemon
  for(QJsonValue jsonEntry : jsonData.array())
  {
    // Create a new event Pokemon entry
    auto entry = new HiddenItemDBEntry(jsonEntry);

    // Add to array
    store.append(entry);
  }

  once = true;
}

void AbstractHiddenItemDB::deepLink()
{
  static bool once = false;
  if(once)
    return;

  for(auto entry : store)
  {
    entry->deepLink();
  }

  once = true;
}

void AbstractHiddenItemDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);

  for(auto el : store)
    el->qmlProtect(engine);
}

AbstractHiddenItemDB::AbstractHiddenItemDB(const QString loadFile)
  : loadFile(loadFile)
{
  load();
}
