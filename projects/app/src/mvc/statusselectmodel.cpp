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

#include "./statusselectmodel.h"

StatusSelectEntry::StatusSelectEntry(QString name, int ind)
  : name(name),
    ind(ind)
{}

StatusSelectModel::StatusSelectModel()
{
  // Populate list
  statusListCache.append(new StatusSelectEntry("-----", 0));
  statusListCache.append(new StatusSelectEntry("Sleep (1)", 1));
  statusListCache.append(new StatusSelectEntry("Sleep (2)", 2));
  statusListCache.append(new StatusSelectEntry("Sleep (3)", 3));
  statusListCache.append(new StatusSelectEntry("Sleep (4)", 4));
  statusListCache.append(new StatusSelectEntry("Sleep (5)", 5));
  statusListCache.append(new StatusSelectEntry("Sleep (6)", 6));
  statusListCache.append(new StatusSelectEntry("Sleep (7)", 7));
  statusListCache.append(new StatusSelectEntry("Poisoned", 8));
  statusListCache.append(new StatusSelectEntry("Burned", 16));
  statusListCache.append(new StatusSelectEntry("Frozen", 32));
  statusListCache.append(new StatusSelectEntry("Paralyzed", 64));
}

int StatusSelectModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return statusListCache.size();
}

QVariant StatusSelectModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= statusListCache.size())
    return QVariant();

  // Get Item from Item List Cache
  auto item = statusListCache.at(index.row());

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

QHash<int, QByteArray> StatusSelectModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "statusInd";
  roles[NameRole] = "statusName";

  return roles;
}

int StatusSelectModel::statusToListIndex(int ind)
{
  int ret = -1;

  for(int i = 0; i < statusListCache.size(); i++) {
    if(ind != statusListCache.at(i)->ind)
      continue;

    ret = i;
    break;
  }

  return ret;
}
