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
 * @file trades.cpp
 * @brief Implementation of TradesDB and TradeDBEntry. See trades.h.
 */

#include <QJsonArray>
#include <QQmlEngine>
#include <pse-common/utility.h>

#ifdef QT_DEBUG
#include <QtDebug>
#endif

#include "./trades.h"
#include "./util/gamedata.h"
#include "./pokemon.h"
#include "./mapsdb.h"
#include "./entries/mapdbentry.h"

TradeDBEntry::TradeDBEntry() {}
TradeDBEntry::TradeDBEntry(QJsonValue& data)
{
  give     = data["give"].toString();
  get      = data["get"].toString();
  textId   = static_cast<var8>(data["textId"].toDouble());
  nickname = data["nickname"].toString();
  if (data["unused"].isBool()) unused = data["unused"].toBool();

  // Location, appended by import_trades.py (additive). Absent on nothing today, but the unused
  // trade legitimately omits mapId/x/y/trader/walks -- it has no NPC -- so each defaults honestly.
  ind        = data["ind"].toInt(-1);
  tradeConst = data["const"].toString();
  mapId      = data["mapId"].toInt(-1);
  x          = data["x"].toInt(0);
  y          = data["y"].toInt(0);
  trader     = data["trader"].toString();
  walks      = data["walks"].toBool(false);
}

void TradeDBEntry::deepLink()
{
  toGive = PokemonDB::inst()->getIndAt(give);
  toGet  = PokemonDB::inst()->getIndAt(get);
#ifdef QT_DEBUG
  if (!toGive) qCritical() << "Trade give:" << give << "could not be deep linked.";
  if (!toGet)  qCritical() << "Trade get:"  << get  << "could not be deep linked.";
#endif
  if (toGive) toGive->toTrades.append(this);
  if (toGet)  toGet->toTrades.append(this);

  // Resolve the map by ID (the game's own number, what import_trades.py validated against
  // maps.json) -- NOT by name: maps.json's `name` is not pret's map constant. -1 is the unused
  // trade, which has no map and correctly links nowhere.
  if (mapId >= 0) {
    toMap = MapsDB::inst()->getIndAt(QString::number(mapId));
#ifdef QT_DEBUG
    if (!toMap) qCritical() << "Trade:" << tradeConst << "map id" << mapId
                            << "could not be deep linked.";
#endif
    if (toMap) toMap->toTrades.append(this);
  }
}

TradesDB* TradesDB::inst()
{
  static TradesDB* _inst = new TradesDB;
  return _inst;
}

const QVector<TradeDBEntry*> TradesDB::getStore() const { return store; }
int TradesDB::getStoreSize() const { return store.size(); }

TradeDBEntry* TradesDB::getStoreAt(int idx) const
{
  if (idx < 0 || idx >= store.size()) return nullptr;
  return store.at(idx);
}

void TradesDB::load()
{
  static bool once = false;
  if (once) return;
  auto jsonData = GameData::inst()->json("trades");
  for (QJsonValue entry : jsonData.array())
    store.append(new TradeDBEntry(entry));
  once = true;
}

void TradesDB::deepLink()
{
  static bool once = false;
  if (once) return;
  for (auto* entry : store)
    entry->deepLink();
  once = true;
}

void TradesDB::qmlProtect(const QQmlEngine* const engine) const
{
  Utility::qmlProtectUtil(this, engine);
}

void TradesDB::qmlRegister() const
{
  static bool once = false;
  if (once) return;
  qmlRegisterUncreatableType<TradesDB>("PSE.DB.TradesDB", 1, 0, "TradesDB", "Can't instantiate in QML");
  once = true;
}

TradesDB::TradesDB()
{
  qmlRegister();
}
