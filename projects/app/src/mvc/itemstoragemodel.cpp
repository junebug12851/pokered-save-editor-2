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
 * @file itemstoragemodel.cpp
 * @brief Implementation of ItemStorageModel. See itemstoragemodel.h.
 */

#include <algorithm>
#include <QCollator>
#include <QDebug>

#include <pse-db/itemsdb.h>
#include "./itemstoragemodel.h"
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/item.h>
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
  connect(this->router, &Router::closeNonModal, this, &ItemStorageModel::pageClosing);
  connect(this->router, &Router::goHome, this, &ItemStorageModel::pageClosing);

  connect(this->items, &ItemStorageBox::beforeItemRelocate, this, &ItemStorageModel::onBeforeRelocate);

  connect(this, &ItemStorageModel::hasCheckedChanged, this, &ItemStorageModel::checkStateDirty);
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
  //qDebug() << "[Pre-Move] From" << from << "to" << to;

  if(from == to)
    return;

  if(to > from)
    to++;

  //qDebug() << "[To-Move] From" << from << "to" << to;

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

bool ItemStorageModel::hasCheckedCached()
{
  return checkedStateDirty;
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

void ItemStorageModel::checkStateDirty()
{
  bool _val = hasChecked();

  if(checkedStateDirty == _val)
    return;

  checkedStateDirty = hasChecked();
  hasCheckedChangedCached();
}

void ItemStorageModel::pageClosing()
{
  if(checkedStateDirty)
    clearCheckedState();
}

void ItemStorageModel::dragReorder(int fromIndex, int toIndex, bool group)
{
  auto& vec = items->items;

  // The items to move, gathered in current box order. A group drag carries the
  // whole checked set; a plain drag carries just the grabbed item.
  QVector<Item*> set;
  if(group)
    set = getChecked();
  else if(fromIndex >= 0 && fromIndex < vec.size())
    set.append(vec.at(fromIndex));

  if(set.isEmpty())
    return;

  // Anchor = the first item at/after the drop slot that ISN'T being moved; the
  // set is re-inserted directly before it (or appended when there is none, e.g.
  // dropping past the last item / onto the empty trailing "+" slot).
  Item* anchor = nullptr;
  for(int i = qBound(0, toIndex, vec.size()); i < vec.size(); i++) {
    if(!set.contains(vec.at(i))) {
      anchor = vec.at(i);
      break;
    }
  }

  // Pull the set out, then splice it back in before the anchor, preserving the
  // set's internal order.
  for(auto el : set)
    vec.removeOne(el);

  int insertAt = (anchor != nullptr) ? vec.indexOf(anchor) : vec.size();
  for(int i = 0; i < set.size(); i++)
    vec.insert(insertAt + i, set.at(i));

  // A whole-vector reshuffle: a model reset is the clean, reliable refresh and
  // sidesteps the beginMoveRows index gymnastics (mirrors PokemonStorageModel's
  // dragReorder and checkedToggleAll). Count is unchanged.
  onReset();
}

void ItemStorageModel::dragTransfer(int fromIndex, int toIndex, bool group)
{
  // Never transfer onto ourselves (both panes are the bag + PC, never the same
  // box, but guard anyway).
  if(otherModel == nullptr || items == otherModel->items)
    return;

  auto src = items;
  auto dst = items->destBox();

  // The items to move, in current (source) box order -- checked set or single.
  QVector<Item*> set;
  if(group)
    set = getChecked();
  else if(fromIndex >= 0 && fromIndex < src->items.size())
    set.append(src->items.at(fromIndex));

  if(set.isEmpty())
    return;

  int inserted = 0;       // how many NEW rows were appended to dst (for the slide)
  bool dstStacked = false; // did we merge into an existing dst stack?

  for(auto el : set) {

    int ind = src->items.indexOf(el);
    if(ind < 0)
      continue;

    // Auto-stack: if the destination already holds this item, merge the moved
    // amount onto the existing stack instead of creating a duplicate row. We
    // stack onto the LAST matching row (Twilight's rule -- if there are e.g. four
    // Antidotes, the bottom one is the stack target). Pre-existing duplicates are
    // left untouched; only the moved item folds in.
    Item* stackTarget = nullptr;
    for(int i = dst->items.size() - 1; i >= 0; i--) {
      if(dst->items.at(i)->ind == el->ind) {
        stackTarget = dst->items.at(i);
        break;
      }
    }

    if(stackTarget != nullptr) {
      // setAmount clamps to the Gen 1 max (99), matching the count field and the
      // rest of the editor -- a stack can't exceed 99, any excess is dropped.
      // No new dst row is created, so this is allowed even when dst is "full".
      stackTarget->setAmount(stackTarget->amount + el->amount);
      src->itemRemove(ind);  // emits itemRemoveChange -> this model's onRemove; deletes el
      dstStacked = true;
      continue;
    }

    // No match in dst -> move as a new row (capacity-guarded). Use `continue`,
    // not `break`: a later item in the set might still stack onto an existing dst
    // row even when dst is row-count full.
    if(dst->items.size() >= dst->itemsMax())
      continue;

    // relocateOne appends to dst's end and emits the remove/insert signals that
    // refresh both panes' models (items need no party/box-style conversion).
    src->relocateOne(ind);
    inserted++;
  }

  // The newly-added (non-stacked) items are the last `inserted` slots of dst.
  // Slide that block to the requested drop slot, clamped to the items that were
  // already there so we insert *among* them (never past the freshly-appended
  // block). Stacked items don't move a row, so they don't participate here.
  if(inserted > 0) {
    int firstAppended = dst->items.size() - inserted;
    int target = qBound(0, toIndex, firstAppended);

    if(target != firstAppended) {
      QVector<Item*> moved = dst->items.mid(firstAppended, inserted);
      dst->items.remove(firstAppended, inserted);
      for(int i = 0; i < moved.size(); i++)
        dst->items.insert(target + i, moved.at(i));
    }
  }

  // One dst refresh covers both the slide reorder and any stacked-count display
  // change (a stack updates an Item amount but emits no row insert, so the dst
  // model needs a reset to re-read it). relocateOne already reset the dst model
  // per new row; an extra reset is idempotent and cheap.
  if(inserted > 0 || dstStacked)
    otherModel->onReset();

  hasCheckedChanged();
}

void ItemStorageModel::deleteItem(int index, bool group)
{
  // A group delete (the item is checked) removes the whole checked set -- this
  // is the replacement for the old footer delete bulk button.
  if(group) {
    checkedDelete();
    return;
  }

  // Single delete (hovering an unchecked item). itemRemove emits
  // itemRemoveChange -> onRemove (begin/endRemoveRows). An item box has no
  // minimum, so no "never empties" guard is needed.
  if(index < 0 || index >= items->items.size())
    return;

  items->itemRemove(index);
}
