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
#include "./itemsmodel.h"

ItemEntryData::ItemEntryData(QString name, int ind)
  : name(name),
    ind(ind)
{}

ItemsModel::ItemsModel()
{
  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  // Add first category
  itemListCache.append(new ItemEntryData("Normal Items", -1));

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
          return collator.compare(item1->name, item2->name) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemEntryData(el->name, el->ind));
  }

  tmp.clear();

  // Add 2nd category
  itemListCache.append(new ItemEntryData("One-Time Items", -1));

  for(auto el : ItemsDB::store) {
    if(el->once && !el->glitch)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->name, item2->name) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemEntryData(el->name, el->ind));
  }

  tmp.clear();

  // Add 3rd category
  itemListCache.append(new ItemEntryData("Glitch Items", -1));

  for(auto el : ItemsDB::store) {
    if(el->glitch)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->name, item2->name) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemEntryData(el->name, el->ind));
  }

  tmp.clear();
}

int ItemsModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return itemListCache.size();
}

QVariant ItemsModel::data(const QModelIndex& index, int role) const
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

QHash<int, QByteArray> ItemsModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "itemInd";
  roles[NameRole] = "itemName";

  return roles;
}
