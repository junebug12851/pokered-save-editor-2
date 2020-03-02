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

#include "./pokemonstoragemodel.h"
#include "../bridge/router.h"
#include "../data/db/pokemon.h"
#include "../data/file/expanded/storage.h"
#include "../data/file/expanded/player/playerpokemon.h"
#include "../data/file/expanded/fragments/item.h"
#include "../data/file/expanded/fragments/pokemonbox.h"
#include "../data/file/expanded/fragments/pokemonparty.h"
#include "../data/file/expanded/fragments/itemstoragebox.h"
#include "../data/file/expanded/fragments/pokemonstoragebox.h"

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

  // Do an initial setup
  switchBox(curBox);
}

void PokemonStorageModel::switchBox(int newBox)
{
  auto box = getCurBox();

  disconnect(box, &PokemonStorageBox::pokemonMoveChange, this, &PokemonStorageModel::onMove);
  disconnect(box, &PokemonStorageBox::pokemonRemoveChange, this, &PokemonStorageModel::onRemove);
  disconnect(box, &PokemonStorageBox::pokemonInsertChange, this, &PokemonStorageModel::onReset);
  disconnect(box, &PokemonStorageBox::pokemonResetChange, this, &PokemonStorageModel::onReset);
  disconnect(box, &PokemonStorageBox::beforePokemonRelocate, this, &PokemonStorageModel::onBeforeRelocate);

  curBox = newBox;
  box = getCurBox();

  connect(box, &PokemonStorageBox::pokemonMoveChange, this, &PokemonStorageModel::onMove);
  connect(box, &PokemonStorageBox::pokemonRemoveChange, this, &PokemonStorageModel::onRemove);
  connect(box, &PokemonStorageBox::pokemonInsertChange, this, &PokemonStorageModel::onReset);
  connect(box, &PokemonStorageBox::pokemonResetChange, this, &PokemonStorageModel::onReset);
  connect(box, &PokemonStorageBox::beforePokemonRelocate, this, &PokemonStorageModel::onBeforeRelocate);
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
  return getCurBox()->pokemon.size() + 1;
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
  for(auto el : getChecked()) {
    int ind = getCurBox()->pokemon.indexOf(el);
    getCurBox()->pokemonRemove(ind);
  }
}

void PokemonStorageModel::checkedTransfer()
{
  // Stop if both models refer to the same box, this is not suppose to happen
  if(getCurBox() == otherModel->getCurBox())
    return;

  for(auto el : getChecked()) {

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
      delete partyEl;
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
  if(checkedStateDirty)
    clearCheckedState();
}
