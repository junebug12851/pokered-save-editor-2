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

#include "./natureselectmodel.h"

NatureSelectEntry::NatureSelectEntry(QString name, int ind)
  : name(name),
    ind(ind)
{}

NatureSelectModel::NatureSelectModel()
{
  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  // Prepare list of natures in correct order
  QString tmpNatures[] = {
    "Hardy",
    "Lonely",
    "Brave",
    "Adamant",
    "Naughty",
    "Bold",
    "Docile",
    "Relaxed",
    "Impish",
    "Lax",
    "Timid",
    "Hasty",
    "Serious",
    "Jolly",
    "Naive",
    "Modest",
    "Mild",
    "Quiet",
    "Bashful",
    "Rash",
    "Calm",
    "Gentle",
    "Sassy",
    "Careful",
    "Quirky"
  };

  // Numerize them automatically
  for(int i = 0; i < 25; i++) {
    natureListCache.append(new NatureSelectEntry(tmpNatures[i], i));
  }

  // Now sort them with their proper ordering preserved from earlier when they
  // were numerized
  std::sort(
      natureListCache.begin(),
      natureListCache.end(),
      [&collator](const NatureSelectEntry* item1, const NatureSelectEntry* item2)
      {
          return collator.compare(item1->name, item2->name) < 0;
      });
}

int NatureSelectModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return natureListCache.size();
}

QVariant NatureSelectModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= natureListCache.size())
    return QVariant();

  // Get Item from Item List Cache
  auto item = natureListCache.at(index.row());

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

QHash<int, QByteArray> NatureSelectModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "natureInd";
  roles[NameRole] = "natureName";

  return roles;
}

int NatureSelectModel::natureToListIndex(int ind)
{
  int ret = -1;

  for(int i = 0; i < natureListCache.size(); i++) {
    if(ind != natureListCache.at(i)->ind)
      continue;

    ret = i;
    break;
  }

  return ret;
}
