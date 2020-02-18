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

#include "../data/db/items.h"
#include "./itemstoragemodel.h"
#include "../data/file/expanded/fragments/itemstoragebox.h"
#include "../data/file/expanded/fragments/item.h"

ItemStorageModel::ItemStorageModel(ItemStorageBox* items)
  : items(items)
{
  connect(this->items, &ItemStorageBox::itemMoveChange, this, &ItemStorageModel::onMove);
  connect(this->items, &ItemStorageBox::itemRemoveChange, this, &ItemStorageModel::onRemove);
  connect(this->items, &ItemStorageBox::itemInsertChange, this, &ItemStorageModel::onInsert);
  connect(this->items, &ItemStorageBox::itemsResetChange, this, &ItemStorageModel::onReset);
}

int ItemStorageModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return items->items.size();
}

QVariant ItemStorageModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= items->items.size())
    return QVariant();

  // Get Item from Item List Cache
  auto item = items->items.at(index.row());

  if(item == nullptr)
    return QVariant();

  // Now return requested information
  if (role == IdRole)
    return item->ind;
  else if (role == CountRole)
    return item->amount;
  else if (role == WorthAllRole)
    return item->worthAll();
  else if (role == WorthEachRole)
    return item->worthOne();

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> ItemStorageModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IdRole] = "itemId";
  roles[CountRole] = "itemCount";
  roles[WorthAllRole] = "itemWorthAll";
  roles[WorthEachRole] = "itemWorthEach";

  return roles;
}

bool ItemStorageModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (!index.isValid())
    return false;

  if (index.row() >= items->items.size())
    return false;

  auto item = items->items.at(index.row());

  if(item == nullptr)
    return false;

  // Now set requested information
  if (role == IdRole) {
    item->ind = value.toInt();
    dataChanged(index, index);
    return true;
  }
  else if (role == CountRole) {
    item->amount = value.toInt();
    dataChanged(index, index);
    return true;
  }

  return false;
}

void ItemStorageModel::onMove(int from, int to)
{
  beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
  endMoveRows();
}

void ItemStorageModel::onRemove(int ind)
{
  beginRemoveRows(QModelIndex(), ind, ind);
  endRemoveRows();
}

void ItemStorageModel::onInsert()
{
  beginInsertRows(QModelIndex(), items->items.size(), items->items.size());
  endInsertRows();
}

void ItemStorageModel::onReset()
{
  beginResetModel();
  endResetModel();
}
