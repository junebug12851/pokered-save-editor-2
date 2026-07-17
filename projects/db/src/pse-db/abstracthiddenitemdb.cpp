/*
  * Copyright 2019 Twilight
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
 * @file abstracthiddenitemdb.cpp
 * @brief Implementation of AbstractHiddenItemDB (base for hidden items/coins).
 *        See abstracthiddenitemdb.h for the documented API.
 */

#include <pse-common/utility.h>
#include <QVector>
#include <QJsonArray>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./abstracthiddenitemdb.h"
#include "./mapsdb.h"
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

HiddenItemDBEntry* AbstractHiddenItemDB::getStoreAt(const int ind) const
{
  if(ind >= store.size())
    return nullptr;

  return store.at(ind);
}

// ⚠️ The once-guards below are PER-INSTANCE members, and that is load-bearing -- they used to be
// `static bool once` locals.
//
// A static local in a base-class method is ONE static for the entire hierarchy, not one per
// subclass. Both HiddenItemsDB and HiddenCoinsDB share this exact load(), so the first singleton
// to be constructed tripped the guard and the second one returned early and loaded NOTHING.
// db.cpp constructs HiddenCoinsDB first, so the casualty was HiddenItemsDB: all 54 hidden items
// silently failed to load, for as long as this code has existed.
//
// It hid well because nothing asserted on it -- `allSubDbsLoadAndCount` only checks `>= 0`, and an
// empty store passes that. tst_db_coverage_fill even recorded the symptom ("the HiddenItems store
// is empty") but read it as a quirk of the test data rather than a bug in the DB.
// Pinned now by tst_db_integrity::bothHiddenDbsLoadTheirOwnData.
void AbstractHiddenItemDB::load()
{
  if(loaded)
    return;

  // Grab the hidden-pickup data (items or coins -- whichever file the subclass named).
  auto jsonData = GameData::inst()->json(loadFile);

  // The index is handed to the entry because it is not decoration: the row's POSITION is the
  // save bit the game tests (FindHiddenItemOrCoinsIndex). Losing it would leave a pickup unable
  // to say which flag it owns.
  for(const QJsonValue& jsonEntry : jsonData.array())
  {
    auto entry = new HiddenItemDBEntry(jsonEntry, store.size(), isCoin);
    store.append(entry);
  }

  loaded = true;
}

void AbstractHiddenItemDB::deepLink()
{
  if(deepLinked)
    return;

  for(auto entry : store)
  {
    entry->deepLink();
  }

  deepLinked = true;
}

void AbstractHiddenItemDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);

  for(auto el : store)
    el->qmlProtect(engine);
}

AbstractHiddenItemDB::AbstractHiddenItemDB(const QString loadFile, bool isCoin)
  : loadFile(loadFile), isCoin(isCoin)
{
  load();
}
