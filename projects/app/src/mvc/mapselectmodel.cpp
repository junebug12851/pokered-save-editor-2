/*
  * Copyright 2020 Twilight
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

#include <algorithm>
#include <QCollator>

#include <pse-db/mapsdb.h>
#include <pse-db/entries/mapdbentry.h>
#include <pse-savefile/expanded/area/areamap.h>
#include "./mapselectmodel.h"

MapSelectEntry::MapSelectEntry(QString name, int ind)
  : name(name),
    ind(ind)
{}

MapSelectModel::MapSelectModel(AreaMap* map)
  : map(map)
{
  // Rebuild Map
  rebuild();

  // Listen for additional re-build events
  connect(map, &AreaMap::curMapChanged, this, &MapSelectModel::rebuild);
}

int MapSelectModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return mapListCache.size();
}

QVariant MapSelectModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= mapListCache.size())
    return QVariant();

  // Get Item from Item List Cache
  auto item = mapListCache.at(index.row());

  if(item == nullptr)
    return QVariant();

  // Now return requested information
  if (role == IndRole)
    return item->ind;
  else if (role == NameRole)
    return item->name;

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> MapSelectModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "mapInd";
  roles[NameRole] = "mapName";

  return roles;
}

int MapSelectModel::mapToListIndex(int ind)
{
  int ret = -1;

  for(int i = 0; i < mapListCache.size(); i++) {
    if(ind != mapListCache.at(i)->ind)
      continue;

    ret = i;
    break;
  }

  return ret;
}

void MapSelectModel::rebuild()
{
  for(auto el : mapListCache)
    delete el;
  mapListCache.clear();

  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  // To prevent re-listing duplicate maps
  QVector<MapDBEntry*> usedMaps;

  // Add current map if there is one
  auto curMapData = map->toCurMap();

  if(curMapData != nullptr) {
    mapListCache.append(new MapSelectEntry("--- Current Map ---", -1));
    mapListCache.append(new MapSelectEntry(curMapData->bestName(), curMapData->getInd()));
    usedMaps.append(curMapData);
  }

  mapListCache.append(new MapSelectEntry("--- Normal Maps ---", -1));

  // Add Daycare first given it's not the current map
  // Daycare is a potential frequently used map
  auto dayCareMap = MapsDB::inst()->getIndAt("Daycare");

  // Last map needs to go at the end, it's a special map
  auto lastMap = MapsDB::inst()->getIndAt("Last Map");

  if(curMapData != dayCareMap) {
    mapListCache.append(new MapSelectEntry(dayCareMap->bestName(), dayCareMap->getInd()));
    usedMaps.append(dayCareMap);
  }

  // Now add in rest

  // Gather normal repeatable items and sort by name, then add into list
  QVector<MapDBEntry*> tmp;

  for(auto el : MapsDB::inst()->getStore()) {
    if(!el->getGlitch() && !el->getSpecial() && el->getIncomplete() == "" && !usedMaps.contains(el) && el != lastMap) {
      tmp.append(el);
      usedMaps.append(el);
    }
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](MapDBEntry* mon1, MapDBEntry* mon2)
      {
          return collator.compare(mon1->bestName(), mon2->bestName()) < 0;
      });

  for(auto el : tmp) {
    mapListCache.append(new MapSelectEntry(el->bestName(), el->getInd()));
  }

  tmp.clear();

  // Add in Last Map right before glitch maps
  mapListCache.append(new MapSelectEntry(lastMap->bestName(), lastMap->getInd()));
  usedMaps.append(lastMap);

  // Add in all other maps
  mapListCache.append(new MapSelectEntry("--- Glitch Maps ---", -1));

  for(auto el : MapsDB::inst()->getStore()) {
    if(!usedMaps.contains(el))
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](MapDBEntry* mon1, MapDBEntry* mon2)
      {
          return collator.compare(mon1->bestName(), mon2->bestName()) < 0;
      });

  for(auto el : tmp) {

    // Get best name and append the incomplete map of name if there is one
    QString name = el->bestName();
    if(el->getIncomplete() != "")
      name += " (" + el->getIncomplete() + ")";

    mapListCache.append(new MapSelectEntry(el->bestName(), el->getInd()));
  }
}

