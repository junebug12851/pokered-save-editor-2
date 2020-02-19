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

#include <algorithm>
#include <QCollator>

#include "../data/db/items.h"
#include "./itemselectmodel.h"

ItemSelectEntryData::ItemSelectEntryData(QString name, int ind)
  : name(name),
    ind(ind)
{}

ItemSelectModel::ItemSelectModel()
{
  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  // Add first category
  itemListCache.append(new ItemSelectEntryData("--- Normal Items ---", -1));

  // Gather normal repeatable items and sort by name, then add into list
  QVector<ItemDBEntry*> tmp;

  for(auto el : ItemsDB::store) {
    if(!el->once && !el->glitch)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->readable, item2->readable) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemSelectEntryData(el->readable, el->ind));
  }

  tmp.clear();

  // Add 2nd category
  itemListCache.append(new ItemSelectEntryData("--- Special Items ---", -1));

  for(auto el : ItemsDB::store) {
    if(el->once && !el->glitch)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->readable, item2->readable) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemSelectEntryData(el->readable, el->ind));
  }

  tmp.clear();

  // Add 3rd category
  itemListCache.append(new ItemSelectEntryData("--- Glitch Items ---", -1));

  for(auto el : ItemsDB::store) {
    if(el->glitch)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->readable, item2->readable) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemSelectEntryData(el->readable, el->ind));
  }

  tmp.clear();
}

int ItemSelectModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return itemListCache.size();
}

QVariant ItemSelectModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= itemListCache.size())
    return QVariant();

  // Get Item from Item List Cache
  auto item = itemListCache.at(index.row());

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

QHash<int, QByteArray> ItemSelectModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "itemSelectInd";
  roles[NameRole] = "itemSelectName";

  return roles;
}

int ItemSelectModel::itemToListIndex(int ind)
{
  int ret = -1;

  for(int i = 0; i < itemListCache.size(); i++) {
    if(ind != itemListCache.at(i)->ind)
      continue;

    ret = i;
    break;
  }

  return ret;
}
