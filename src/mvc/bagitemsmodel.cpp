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
#include "./bagitemsmodel.h"
#include "../data/file/expanded/player/playeritems.h"
#include "../data/file/expanded/fragments/item.h"

BagItemsModel::BagItemsModel(PlayerItems* items)
  : items(items)
{
  connect(this->items, &PlayerItems::bagItemSwapChange, this, &BagItemsModel::onSwap);
  connect(this->items, &PlayerItems::bagItemRemoveChange, this, &BagItemsModel::onRemove);
  connect(this->items, &PlayerItems::bagItemInsertChange, this, &BagItemsModel::onInsert);
  connect(this->items, &PlayerItems::bagItemResetChange, this, &BagItemsModel::onReset);
}

int BagItemsModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return items->bagItems.size();
}

QVariant BagItemsModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= items->bagItems.size())
    return QVariant();

  // Get Item from Item List Cache
  auto item = items->bagItems.at(index.row());

  if(item == nullptr)
    return QVariant();

  // Now return requested information
  if (role == IdRole)
    return item->ind;
  else if (role == CountRole)
    return item->amount;

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> BagItemsModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IdRole] = "itemId";
  roles[CountRole] = "itemCount";

  return roles;
}

Qt::ItemFlags BagItemsModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool BagItemsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (index.isValid() && role == Qt::EditRole) {

    stringList.replace(index.row(), value.toString());
    emit dataChanged(index, index, {role});
    return true;
  }

  return false;
}

void BagItemsModel::onSwap(int from, int to)
{

}

void BagItemsModel::onRemove(int ind)
{

}

void BagItemsModel::onInsert()
{

}

void BagItemsModel::onReset()
{

}
