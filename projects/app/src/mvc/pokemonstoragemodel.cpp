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
 * @file pokemonstoragemodel.cpp
 * @brief Implementation of PokemonStorageModel. See pokemonstoragemodel.h.
 */

#include <algorithm>
#include <QCollator>
#include <QDebug>
#include <QQmlEngine>

#include "./pokemonstoragemodel.h"
#include "../bridge/router.h"
#include <pse-savefile/qmlownership.h>
#include <pse-db/pokemon.h>
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/player/playerpokemon.h>
#include <pse-savefile/expanded/fragments/item.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include <pse-savefile/expanded/fragments/pokemonparty.h>
#include <pse-savefile/expanded/fragments/itemstoragebox.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>

PokemonStorageModel::PokemonStorageModel(
    Router* router,
    Storage* storage,
    PlayerPokemon* party)
  : router(router),
    storage(storage),
    party(party)
{
  connect(this->router, &Router::closeNonModal, this, &PokemonStorageModel::pageClosing);
  connect(this->router, &Router::goHome, this, &PokemonStorageModel::pageClosing);
  connect(this, &PokemonStorageModel::hasCheckedChanged, this, &PokemonStorageModel::checkStateDirty);
  connect(this, &PokemonStorageModel::curBoxChanged, this, &PokemonStorageModel::onReset);

  // Do an initial setup
  switchBox(curBox, true);
}

void PokemonStorageModel::switchBox(int newBox, bool force)
{
  // Do nothing if it's the same box
  if(!force && curBox == newBox)
    return;

  // Clear the OUTGOING box's checks: changing boxes should NOT keep the selection
  // (Twilight's UX rule -- checks only persist across the Pokemon-detail editor
  // round-trip, handled by Pokemon.qml's Component.onDestruction, not here).
  if(checkedStateDirty)
    clearCheckedState();

  // Get old current box
  auto box = getCurBox();

  // Disconnect from it
  disconnect(box, &PokemonStorageBox::pokemonMoveChange, this, &PokemonStorageModel::onMove);
  disconnect(box, &PokemonStorageBox::pokemonRemoveChange, this, &PokemonStorageModel::onRemove);
  disconnect(box, &PokemonStorageBox::pokemonInsertChange, this, &PokemonStorageModel::onReset);
  disconnect(box, &PokemonStorageBox::pokemonResetChange, this, &PokemonStorageModel::onReset);
  disconnect(box, &PokemonStorageBox::beforePokemonRelocate, this, &PokemonStorageModel::onBeforeRelocate);

  // Change boxes and retrieve new box
  curBox = newBox;
  box = getCurBox();

  // Connect to it
  connect(box, &PokemonStorageBox::pokemonMoveChange, this, &PokemonStorageModel::onMove);
  connect(box, &PokemonStorageBox::pokemonRemoveChange, this, &PokemonStorageModel::onRemove);
  connect(box, &PokemonStorageBox::pokemonInsertChange, this, &PokemonStorageModel::onReset);
  connect(box, &PokemonStorageBox::pokemonResetChange, this, &PokemonStorageModel::onReset);
  connect(box, &PokemonStorageBox::beforePokemonRelocate, this, &PokemonStorageModel::onBeforeRelocate);

  // Announce change
  curBoxChanged();
}

PokemonStorageBox* PokemonStorageModel::getCurBox() const
{
  return getBox(curBox);
}

PokemonStorageBox* PokemonStorageModel::getBox(int box) const
{
  if(box == PartyBox)
    return party;
  else
    return storage->boxAt(box);
}

int PokemonStorageModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return (getCurBox()->isFull())
      ? getCurBox()->pokemon.size()
      : getCurBox()->pokemon.size() + 1;
}

QVariant PokemonStorageModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() > getCurBox()->pokemon.size())
    return QVariant();

  if(index.row() == (getCurBox()->pokemon.size()))
    return getPlaceHolderData(role);

  // Get Item from Item List Cache
  auto mon = getCurBox()->pokemon.at(index.row());
  auto monData = mon->toData();

  // Even for glitch Pokemon, there should still be a data entry
  if(mon == nullptr || monData == nullptr)
    return QVariant();

  // Now return requested information
  if (role == IndRole)
    return monData->ind;
  else if (role == DexRole)
    return (monData->pokedex) ? *monData->pokedex : -1;
  else if (role == NameRole)
    return monData->readable;
  else if (role == CheckedRole)
    return mon->property(isCheckedKey).toBool();
  else if (role == PlaceholderRole)
    return false;
  else if (role == NicknameRole)
    return mon->nickname;
  else if (role == LevelRole)
    return mon->level;
  else if (role == IsShinyRole)
    return mon->isShiny();
  else if (role == IsPartyRole)
    return !mon->isBoxMon();
  else if (role == HpRole)
    return mon->hp;
  else if (role == HpMaxRole)
    return mon->hpStat();
  else if (role == StatusRole)
    return mon->status;

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> PokemonStorageModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "itemInd";
  roles[DexRole] = "itemDex";
  roles[NameRole] = "itemName";
  roles[CheckedRole] = "itemChecked";
  roles[PlaceholderRole] = "itemIsPlaceholder";
  roles[NicknameRole] = "itemNickname";
  roles[LevelRole] = "itemLevel";
  roles[IsShinyRole] = "itemIsShiny";
  roles[IsPartyRole] = "itemIsParty";
  roles[HpRole] = "itemHp";
  roles[HpMaxRole] = "itemHpMax";
  roles[StatusRole] = "itemStatus";

  return roles;
}

bool PokemonStorageModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if(!index.isValid())
    return false;

  if(index.row() >= getCurBox()->pokemon.size())
    return false;

  auto mon = getCurBox()->pokemon.at(index.row());

  if(mon == nullptr)
    return false;

  // Now set requested information
  if(role == CheckedRole) {
    mon->setProperty(isCheckedKey, value.toBool());
    hasCheckedChanged();
    dataChanged(index, index);
    return true;
  }

  return false;
}

QVariant PokemonStorageModel::getPlaceHolderData(int role) const
{
  if (role == IndRole)
    return -1;
  else if (role == DexRole)
    return -1;
  else if (role == NameRole)
    return "";
  else if (role == CheckedRole)
    return false;
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
void PokemonStorageModel::onMove(int from, int to)
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

void PokemonStorageModel::onRemove(int ind)
{
  beginRemoveRows(QModelIndex(), ind, ind);
  endRemoveRows();
  hasCheckedChanged();
}

void PokemonStorageModel::onInsert()
{
  // Doesn't work, no idea why
  beginInsertRows(QModelIndex(), getCurBox()->pokemon.size()+1, getCurBox()->pokemon.size()+1);
  endInsertRows();
}

void PokemonStorageModel::onReset()
{
  beginResetModel();
  endResetModel();
  hasCheckedChanged();
}

bool PokemonStorageModel::hasChecked()
{
  bool ret = false;

  for(auto el : getCurBox()->pokemon) {
    if(el->property(isCheckedKey).toBool() == true)
      ret = true;
  }

  return ret;
}

bool PokemonStorageModel::hasCheckedCached()
{
  return checkedStateDirty;
}

QVector<PokemonBox*> PokemonStorageModel::getChecked()
{
  QVector<PokemonBox*> ret;

  for(auto el : getCurBox()->pokemon) {
    if(el->property(isCheckedKey).toBool() == true)
      ret.append(el);
  }

  return ret;
}

void PokemonStorageModel::clearCheckedState()
{
  for(auto el : getCurBox()->pokemon) {
    el->setProperty(isCheckedKey, false);
  }

  hasCheckedChanged();
}

void PokemonStorageModel::clearCheckedStateGone()
{
  for(int i = 0; i < getCurBox()->pokemon.size(); i++) {
    auto el = getCurBox()->pokemon.at(i);
    el->setProperty(isCheckedKey, false);
    dataChanged(index(i), index(i));
  }

  hasCheckedChanged();
}

void PokemonStorageModel::checkedMoveToTop()
{
  for(auto el : getChecked()) {
    int ind = getCurBox()->pokemon.indexOf(el);
    getCurBox()->pokemonMove(ind, 0);
  }
}

void PokemonStorageModel::checkedMoveUp()
{
  for(auto el : getChecked()) {
    int ind = getCurBox()->pokemon.indexOf(el);
    getCurBox()->pokemonMove(ind, ind - 1);
  }
}

void PokemonStorageModel::checkedMoveDown()
{
  auto checkedItems = getChecked();

  // For stupid ListView, these need to be done in reverse
  // If moving down
  for(int i = checkedItems.size() - 1; i >= 0; i--) {
    auto el = checkedItems.at(i);
    int ind = getCurBox()->pokemon.indexOf(el);
    getCurBox()->pokemonMove(ind, ind + 1);
  }
}

void PokemonStorageModel::checkedMoveToBottom()
{
  auto checkedItems = getChecked();

  // For stupid ListView, these need to be done in reverse
  // If moving down
  for(int i = checkedItems.size() - 1; i >= 0; i--) {
    auto el = checkedItems.at(i);
    int ind = getCurBox()->pokemon.indexOf(el);
    getCurBox()->pokemonMove(ind, getCurBox()->pokemon.size() - 1);
  }
}

void PokemonStorageModel::checkedDelete()
{
  bool annPlaceholder = getCurBox()->pokemonCount() == getCurBox()->pokemonMax();

  for(auto el : getChecked()) {

    // Stop if the party is down to one Pokemon
    if(getCurBox() == party && party->pokemonCount() <= 1)
      break;

    int ind = getCurBox()->pokemon.indexOf(el);
    getCurBox()->pokemonRemove(ind);
  }

  // Announce the opening of the placeholder
  if(annPlaceholder) {
    beginInsertRows(QModelIndex(), getCurBox()->pokemonCount(), getCurBox()->pokemonCount());
    endInsertRows();
  }
}

void PokemonStorageModel::checkedTransfer()
{
  // Stop if both models refer to the same box, this is not suppose to happen
  if(getCurBox() == otherModel->getCurBox())
    return;

  for(auto el : getChecked()) {

    // We don't want to convert to and from data structures if we can't move
    // the pokemon. Perform checks to make sure it's possible.

    // Stop if the party is down to one Pokemon
    if(getCurBox() == party && party->pokemonCount() <= 1)
      break;

    // Stop here if the other side is full
    if(otherModel->getCurBox()->isFull())
      break;

    // Get index
    int ind = getCurBox()->pokemon.indexOf(el);

    // If we're converting to a PokemonParty we auto-destroy the box pokemon and
    // produce the same Pokemon with the PokemonParty extensions. If the other
    // way around, we can't auto-destroy it so we manually do the conversion and
    // destroying
    // We also have to replace the newly converted Pokemon in the array before
    // relocating it
    if(otherModel->getCurBox()->isParty) {
      el = PokemonParty::convertToParty(el);
      getCurBox()->pokemon.replace(ind, el);
    }
    else if(!otherModel->getCurBox()->isParty && getCurBox()->isParty) {
      auto partyEl = (PokemonParty*)el;
      el = partyEl->toBoxData();
      partyEl->deleteLater();
      getCurBox()->pokemon.replace(ind, el);
    }

    // Now do the relocate
    getCurBox()->relocateOne(otherModel->getCurBox(), ind);
  }

  hasCheckedChanged();
}

void PokemonStorageModel::checkedToggleAll()
{
  if(getCurBox()->pokemon.size() == 0)
    return;

  bool allValue = !getCurBox()->pokemon.at(0)->property(isCheckedKey).toBool();

  for(int i = 0; i < getCurBox()->pokemon.size(); i++) {
    auto el = getCurBox()->pokemon.at(i);
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

void PokemonStorageModel::onBeforeRelocate(PokemonBox* mon)
{
  mon->setProperty(isCheckedKey, false);
  hasCheckedChanged();
}

void PokemonStorageModel::checkStateDirty()
{
  bool _val = hasChecked();

  if(checkedStateDirty == _val)
    return;

  checkedStateDirty = _val;
  hasCheckedChangedCached();
}

void PokemonStorageModel::pageClosing()
{
  // Intentionally does NOT clear the checked state. This fires on
  // router.closeNonModal (which includes closing a mon's *detail editor* and
  // returning to the storage screen) and on goHome -- clearing here wiped every
  // checkbox the moment the user opened a mon and came back, which read as
  // "checkboxes lose their selection". Checked state is per-mon and now persists
  // for the file's lifetime (Twilight's call); transfers/deletes still clear the
  // specific mons they touch. Kept as a hook in case a future close action needs it.
}

PokemonBox* PokemonStorageModel::getBoxMon(int index)
{
  // qmlCppOwned: these mons are owned by the storage box in C++ for its
  // lifetime. Without this the parentless Q_INVOKABLE return defaults to
  // JavaScriptOwnership, so QML GCs the mon when the details editor closes --
  // leaving a dangling pointer in getCurBox()->pokemon that hasChecked()
  // dereferences on the next onReset() -> use-after-free crash. (Opening the
  // editor and leaving reproduced it.) See qt6-patterns.md / qmlownership.h.
  return qmlCppOwned(getCurBox()->pokemon.at(index));
}

PokemonParty* PokemonStorageModel::getPartyMon(int index)
{
  auto mon = getCurBox()->pokemon.at(index);
  if(mon->isBoxMon())
    return nullptr;

  // qmlCppOwned: see getBoxMon() above -- same QML-GC use-after-free guard.
  return qmlCppOwned((PokemonParty*)mon);
}

void PokemonStorageModel::dragReorder(int fromIndex, int toIndex, bool group)
{
  auto& vec = getCurBox()->pokemon;

  // The mons to move, gathered in current box order. A group drag carries the
  // whole checked set; a plain drag carries just the grabbed mon.
  QVector<PokemonBox*> set;
  if(group)
    set = getChecked();
  else if(fromIndex >= 0 && fromIndex < vec.size())
    set.append(vec.at(fromIndex));

  if(set.isEmpty())
    return;

  // Anchor = the first mon at/after the drop slot that ISN'T being moved; the
  // set is re-inserted directly before it (or appended when there is none, e.g.
  // dropping past the last mon / onto the empty trailing slot).
  PokemonBox* anchor = nullptr;
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
  // sidesteps the beginMoveRows index gymnastics (count is unchanged, so the
  // box-selector counts -- the only pokemonChanged listener -- need no update).
  // Mirrors checkedToggleAll's external-change reset.
  onReset();
}

void PokemonStorageModel::dragTransfer(int fromIndex, int toIndex, bool group)
{
  // Never transfer onto ourselves (both panes showing the same box).
  if(otherModel == nullptr || getCurBox() == otherModel->getCurBox())
    return;

  auto src = getCurBox();
  auto dst = otherModel->getCurBox();

  // The mons to move, in current (source) box order -- checked set or single.
  QVector<PokemonBox*> set;
  if(group)
    set = getChecked();
  else if(fromIndex >= 0 && fromIndex < src->pokemon.size())
    set.append(src->pokemon.at(fromIndex));

  if(set.isEmpty())
    return;

  int inserted = 0;

  for(auto el : set) {

    // Same guards as checkedTransfer: keep the party non-empty, never overflow
    // the destination.
    if(src == party && party->pokemonCount() <= 1)
      break;

    if(dst->isFull())
      break;

    int ind = src->pokemon.indexOf(el);
    if(ind < 0)
      continue;

    // Party and box records aren't interchangeable on disk -- convert to the
    // destination's format before relocating (mirrors checkedTransfer exactly).
    if(dst->isParty) {
      el = PokemonParty::convertToParty(el);
      src->pokemon.replace(ind, el);
    }
    else if(!dst->isParty && src->isParty) {
      auto partyEl = (PokemonParty*)el;
      el = partyEl->toBoxData();
      partyEl->deleteLater();
      src->pokemon.replace(ind, el);
    }

    // relocateOne appends to dst's end and emits the remove/insert signals that
    // refresh both panes' models.
    src->relocateOne(dst, ind);
    inserted++;
  }

  // The transferred mons are now the last `inserted` slots of dst. Slide that
  // block to the requested drop slot, clamped to the mons that were already
  // there so we insert *among* them (never past the freshly-appended block).
  if(inserted > 0) {
    int firstAppended = dst->pokemon.size() - inserted;
    int target = qBound(0, toIndex, firstAppended);

    if(target != firstAppended) {
      QVector<PokemonBox*> moved = dst->pokemon.mid(firstAppended, inserted);
      dst->pokemon.remove(firstAppended, inserted);
      for(int i = 0; i < moved.size(); i++)
        dst->pokemon.insert(target + i, moved.at(i));

      otherModel->onReset();
    }
  }

  hasCheckedChanged();
}

void PokemonStorageModel::deleteMon(int index, bool group)
{
  // A group delete (the mon is checked) removes the whole checked set -- this is
  // the replacement for the old footer "release" bulk button.
  if(group) {
    checkedDelete();
    return;
  }

  // Single delete (hovering an unchecked mon).
  auto box = getCurBox();

  if(index < 0 || index >= box->pokemon.size())
    return;

  // Keep the party non-empty (same guard as checkedDelete).
  if(box == party && party->pokemonCount() <= 1)
    return;

  // If the box was full it had no trailing "+" placeholder row; removing one mon
  // opens it, so announce that new row (mirrors checkedDelete).
  bool annPlaceholder = box->pokemonCount() == box->pokemonMax();

  box->pokemonRemove(index); // emits pokemonRemoveChange -> onRemove (begin/endRemoveRows)

  if(annPlaceholder) {
    beginInsertRows(QModelIndex(), box->pokemonCount(), box->pokemonCount());
    endInsertRows();
  }
}
