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
 * @file mapstatesdb.cpp
 * @brief Implementation of MapStatesDB. See mapstatesdb.h for the documented API.
 */

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "./mapstatesdb.h"
#include "./util/gamedata.h"

const MapStateStage* MapStateBlueprint::stage(const QString& id) const
{
  for (const auto& st : stages)
    if (st.id == id)
      return &st;
  return nullptr;
}

MapStatesDB* MapStatesDB::inst()
{
  static MapStatesDB* _inst = new MapStatesDB;
  return _inst;
}

const QHash<int, MapStateBlueprint*>& MapStatesDB::getStore() const
{
  return store;
}

int MapStatesDB::getStoreSize() const
{
  return store.size();
}

const MapStateBlueprint* MapStatesDB::at(int mapInd) const
{
  return store.value(mapInd, nullptr);
}

static QVector<MapStateEv> readEvs(const QJsonArray& arr)
{
  QVector<MapStateEv> out;
  out.reserve(arr.size());
  for (const QJsonValue v : arr) {
    const auto o = v.toObject();
    out.append({o.value("ind").toInt(-1), o.value("name").toString(),
                o.value("owned").toBool(true)});
  }
  return out;
}

void MapStatesDB::load()
{
  static bool once = false;
  if (once)
    return;

  // The index names every blueprint file; each file is a whole map's progression.
  const auto index = GameData::inst()->json("map-states/_index").array();
  for (const QJsonValue idxVal : index) {
    const auto idx = idxVal.toObject();
    QString file = idx.value("file").toString();      // "PalletTown.json"
    file.chop(5);                                     // -> "PalletTown"
    const auto doc = GameData::inst()->json("map-states/" + file).object();
    if (doc.isEmpty())
      continue;                                       // a missing file is a data bug; skip, never invent

    auto* bp = new MapStateBlueprint;
    bp->mapInd = doc.value("mapInd").toInt(-1);
    bp->scriptSlot = doc.value("scriptSlot").isNull()
                       ? -1 : doc.value("scriptSlot").toInt(-1);
    bp->curated = doc.value("curated").toBool(false);
    bp->mapName = doc.value("map").toString();
    const auto entry = doc.value("entry").toObject();
    bp->entryX = entry.value("x").toInt(0);
    bp->entryY = entry.value("y").toInt(0);
    const auto prog = doc.value("progression").toObject();
    bp->start = prog.value("start").toString();
    bp->end = prog.value("end").toString();
    for (const QJsonValue o : prog.value("order").toArray())
      bp->order.append(o.toString());
    bp->branches = prog.value("branches").toObject().toVariantMap();
    bp->messy = prog.value("messy").toBool(false);
    bp->notes = prog.value("notes").toString();
    bp->scriptValues = doc.value("scriptValues").toArray().toVariantList();

    for (const QJsonValue sVal : doc.value("states").toArray()) {
      const auto s = sVal.toObject();
      MapStateStage st;
      st.id = s.value("id").toString();
      st.kind = s.value("kind").toString();
      st.name = s.value("name").toString();
      st.desc = s.value("desc").toString();
      st.timeline = s.value("timeline").toString();
      st.triggerText = s.value("trigger").toObject().value("text").toString();
      st.script = s.value("script").toInt(0);
      st.scriptName = s.value("scriptName").toString();
      st.derived = s.value("derived").toBool(false);
      st.advance = s.value("advance").toArray().toVariantList();
      if (s.contains("save")) {
        st.hasSave = true;
        const auto sv = s.value("save").toObject();
        const auto evs = sv.value("events").toObject();
        st.set = readEvs(evs.value("set").toArray());
        st.cleared = readEvs(evs.value("cleared").toArray());
        for (const QJsonValue mVal : sv.value("missables").toArray()) {
          const auto m = mVal.toObject();
          st.missables.append({m.value("ind").toInt(-1), m.value("name").toString(),
                               m.value("state").toString() == "hide"});
        }
        for (const QJsonValue b : sv.value("badges").toArray())
          st.badges.append(b.toString());
        for (const QJsonValue n : sv.value("notes").toArray())
          st.notes.append(n.toString());
      }
      bp->stages.append(st);
    }

    // The badge universe: every badge ANY stage of this map touches. Applying a
    // stage sets its listed badges and clears the rest of the universe -- rolling a
    // gym back un-awards its badge, and no other map's badges are ever touched.
    static const QStringList badgeNames = {
      "BOULDERBADGE", "CASCADEBADGE", "THUNDERBADGE", "RAINBOWBADGE",
      "SOULBADGE", "MARSHBADGE", "VOLCANOBADGE", "EARTHBADGE"};
    for (const auto& st : bp->stages)
      for (const auto& b : st.badges) {
        const int bit = int(badgeNames.indexOf(b));
        if (bit >= 0)
          bp->badgeUniverse |= quint8(1u << bit);
      }

    if (bp->mapInd >= 0)
      store.insert(bp->mapInd, bp);
    else
      delete bp;
  }

  once = true;
}
