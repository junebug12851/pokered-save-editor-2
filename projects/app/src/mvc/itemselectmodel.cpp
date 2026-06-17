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

/**
 * @file itemselectmodel.cpp
 * @brief Implementation of ItemSelectModel. See itemselectmodel.h.
 */

#include <algorithm>
#include <QCollator>

#include <pse-db/itemsdb.h>
#include <pse-db/entries/itemdbentry.h>
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

  for(auto el : ItemsDB::inst()->getStore()) {
    if(!el->getOnce() && !el->getGlitch())
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->getReadable(), item2->getReadable()) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemSelectEntryData(el->getReadable(), el->getInd()));
  }

  tmp.clear();

  // Add 2nd category
  itemListCache.append(new ItemSelectEntryData("--- Special Items ---", -1));

  for(auto el : ItemsDB::inst()->getStore()) {
    if(el->getOnce() && !el->getGlitch())
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->getReadable(), item2->getReadable()) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemSelectEntryData(el->getReadable(), el->getInd()));
  }

  tmp.clear();

  // Add 3rd category
  itemListCache.append(new ItemSelectEntryData("--- Glitch Items ---", -1));

  for(auto el : ItemsDB::inst()->getStore()) {
    if(el->getGlitch())
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const ItemDBEntry* item1, const ItemDBEntry* item2)
      {
          return collator.compare(item1->getReadable(), item2->getReadable()) < 0;
      });

  for(auto el : tmp) {
    itemListCache.append(new ItemSelectEntryData(el->getReadable(), el->getInd()));
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
  else if (role == InfoRole)
    return infoForInd(item->ind);

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> ItemSelectModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "itemSelectInd";
  roles[NameRole] = "itemSelectName";
  roles[InfoRole] = "itemSelectInfo";

  return roles;
}

QString ItemSelectModel::infoForInd(int ind) const
{
  if(ind < 0)
    return QString();
  auto el = ItemsDB::inst()->getIndAt(QString::number(ind));
  return (el != nullptr) ? el->getInfo() : QString();
}

int ItemSelectModel::itemToListIndex(int ind)
{
  int ret = -1;

  for(int i = 0; i < itemListCache.size(); i++) {
    if(itemListCache.at(i)->ind == ind) {
      ret = i;
      break;
    }
  }

  return ret;
}
