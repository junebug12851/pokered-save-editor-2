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
#include <QDebug>

#include "../data/db/items.h"
#include "./itemstoragemodel.h"
#include "../data/file/expanded/fragments/itemstoragebox.h"
#include "../data/file/expanded/fragments/item.h"
#include "../bridge/router.h"

ItemStorageModel::ItemStorageModel(ItemStorageBox* items, Router* router)
  : items(items),
    router(router)
{
  // Connect signals from the box
  connect(this->items, &ItemStorageBox::itemMoveChange, this, &ItemStorageModel::onMove);
  connect(this->items, &ItemStorageBox::itemRemoveChange, this, &ItemStorageModel::onRemove);

  // Doesn't work, no idea why
  //connect(this->items, &ItemStorageBox::itemInsertChange, this, &ItemStorageModel::onInsert);

  // A hack because insert doesn't work as advertised, yet another millionth QML
  // and Qt Gotcha I could spend hours, days, weeks, or months trying ti fix
  connect(this->items, &ItemStorageBox::itemInsertChange, this, &ItemStorageModel::onReset);

  connect(this->items, &ItemStorageBox::itemsResetChange, this, &ItemStorageModel::onReset);

  // Clear checked on non-modal screen change. The checked state is completely
  // temporary, it should not persist between non-modal screens
  connect(this->router, &Router::closeNonModal, this, &ItemStorageModel::clearCheckedState);
  connect(this->router, &Router::goHome, this, &ItemStorageModel::clearCheckedState);

  connect(this->items, &ItemStorageBox::beforeItemRelocate, this, &ItemStorageModel::onBeforeRelocate);
}

int ItemStorageModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return items->items.size() + 1;
}

QVariant ItemStorageModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() > items->items.size())
    return QVariant();

  if(index.row() == (items->items.size()))
    return getPlaceHolderData(role);

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
  else if (role == CheckedRole)
    return item->property(isCheckedKey).toBool();
  else if (role == PlaceholderRole)
    return false;

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
  roles[CheckedRole] = "itemChecked";
  roles[PlaceholderRole] = "itemIsPlaceholder";

  return roles;
}

bool ItemStorageModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if(!index.isValid())
    return false;

  if(index.row() >= items->items.size())
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
  else if (role == CheckedRole) {
    item->setProperty(isCheckedKey, value.toBool());
    hasCheckedChanged();
    dataChanged(index, index);
    return true;
  }

  return false;
}

QVariant ItemStorageModel::getPlaceHolderData(int role) const
{
  if (role == IdRole)
    return -1;
  else if (role == CountRole)
    return 0;
  else if (role == WorthAllRole)
    return 0;
  else if (role == WorthEachRole)
    return 0;
  else if (role == CheckedRole)
    return 0;
  else if (role == PlaceholderRole)
    return true;

  return QVariant();
}

// QML ListView is the worst I've ever used in any language, so many hours and
// days have been lost fixing ListView issues, bugs, and gotchas
//
// beginMoveRows is the only way to move rows and it's terminology is different
// than other Qt terminology.
// "to" is 1 past the item to move to, elsewhere in Qt "to" is the index to move
// to. This creates all kinds of havoc as the two cannot properly communicate
// and translation is very error prone leading to all my problems.
void ItemStorageModel::onMove(int from, int to)
{
  // I'm convinced I'm never going to be able to remove these for the life of
  // the entire program because I'm convinced that seeking out ListView bugs
  // specifically and only related to beginMoveRow bugs will never end. Ever!
  qDebug() << "[Pre-Move] From" << from << "to" << to;

  if(from == to)
    return;

  if(to > from)
    to++;

  qDebug() << "[To-Move] From" << from << "to" << to;

  beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
  endMoveRows();
}

void ItemStorageModel::onRemove(int ind)
{
  beginRemoveRows(QModelIndex(), ind, ind);
  endRemoveRows();
  hasCheckedChanged();
}

void ItemStorageModel::onInsert()
{
  // Doesn't work, no idea why
  beginInsertRows(QModelIndex(), items->items.size()+1, items->items.size()+1);
  endInsertRows();
}

void ItemStorageModel::onReset()
{
  beginResetModel();
  endResetModel();
  hasCheckedChanged();
}

bool ItemStorageModel::hasChecked()
{
  bool ret = false;

  for(auto el : items->items) {
    if(el->property(isCheckedKey).toBool() == true)
      ret = true;
  }

  return ret;
}

QVector<Item*> ItemStorageModel::getChecked()
{
  QVector<Item*> ret;

  for(auto el : items->items) {
    if(el->property(isCheckedKey).toBool() == true)
      ret.append(el);
  }

  return ret;
}

void ItemStorageModel::clearCheckedState()
{
  for(auto el : items->items) {
    el->setProperty(isCheckedKey, false);
  }

  hasCheckedChanged();
}

void ItemStorageModel::clearCheckedStateGone()
{
  for(int i = 0; i < items->items.size(); i++) {
    auto el = items->items.at(i);
    el->setProperty(isCheckedKey, false);
    dataChanged(index(i), index(i));
  }

  hasCheckedChanged();
}

void ItemStorageModel::checkedMoveToTop()
{
  for(auto el : getChecked()) {
    int ind = items->items.indexOf(el);
    items->itemMove(ind, 0);
  }
}

void ItemStorageModel::checkedMoveUp()
{
  for(auto el : getChecked()) {
    int ind = items->items.indexOf(el);
    items->itemMove(ind, ind - 1);
  }
}

void ItemStorageModel::checkedMoveDown()
{
  auto checkedItems = getChecked();

  // For stupid ListView, these need to be done in reverse
  // If moving down
  for(int i = checkedItems.size() - 1; i >= 0; i--) {
    auto el = checkedItems.at(i);
    int ind = items->items.indexOf(el);
    items->itemMove(ind, ind + 1);
  }
}

void ItemStorageModel::checkedMoveToBottom()
{
  auto checkedItems = getChecked();

  // For stupid ListView, these need to be done in reverse
  // If moving down
  for(int i = checkedItems.size() - 1; i >= 0; i--) {
    auto el = checkedItems.at(i);
    int ind = items->items.indexOf(el);
    items->itemMove(ind, items->items.size() - 1);
  }
}

void ItemStorageModel::checkedDelete()
{
  for(auto el : getChecked()) {
    int ind = items->items.indexOf(el);
    items->itemRemove(ind);
  }
}

void ItemStorageModel::checkedTransfer()
{
  for(auto el : getChecked()) {
    int ind = items->items.indexOf(el);
    items->relocateOne(ind);
  }

  hasCheckedChanged();
}

void ItemStorageModel::checkedToggleAll()
{
  if(items->items.size() == 0)
    return;

  bool allValue = !items->items.at(0)->property(isCheckedKey).toBool();

  for(int i = 0; i < items->items.size(); i++) {
    auto el = items->items.at(i);
    el->setProperty(isCheckedKey, allValue);
  }

  // As far as I can tell there is no way to force a ListView row to be
  // re-created. This means dataChanged is completely and utterly useless. I've
  // tried all kinds of tricks, I've searched Google for hours, I've even tried
  // a hack where I remove and re-insert all rows to force the rows to be
  // re-created. I'm left with no other option but to completely destroy the
  // model and re-create it.
  //
  // The issue stems from another Qt gotcha. Models are expected to change only
  // from their delegates, they are never expected to externally change and as
  // such any external changes have no way of being reflected in the model
  // without a reset.
  beginResetModel();
  endResetModel();

  hasCheckedChanged();
}

void ItemStorageModel::onBeforeRelocate(Item* item)
{
  item->setProperty(isCheckedKey, false);
  hasCheckedChanged();
}
