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

#include "./pokemonboxselectmodel.h"
#include "./pokemonstoragemodel.h"
#include "../data/file/expanded/storage.h"
#include "../data/file/expanded/player/playerpokemon.h"
#include "../data/file/expanded/fragments/pokemonbox.h"
#include "../data/file/expanded/fragments/pokemonparty.h"
#include "../data/file/expanded/fragments/pokemonstoragebox.h"

PokemonBoxSelectModel::PokemonBoxSelectModel(PokemonStorageModel* pairedModel)
  : pairedModel(pairedModel)
{
  storage = pairedModel->storage;
  party = pairedModel->party;

  // The box list has to know all the up-to-date information on all the pokemon
  // boxes because the list that is displayed contains more information than
  // just a box number

  // Subscribe to all changes in all PokemonStorageBox's
  for(int i = 0; i < 12; i++) {
    connect(storage->boxAt(i), &PokemonStorageBox::pokemonChanged, this, &PokemonBoxSelectModel::onBoxChange);
  }

  // Subscribe to all changes in the players party
  connect(party, &PlayerPokemon::pokemonChanged, this, &PokemonBoxSelectModel::onBoxChange);

  // Subscribe to certain changes in storage
  connect(storage, &Storage::pokemonChanged, this, &PokemonBoxSelectModel::onBoxChange);
  connect(storage, &Storage::curBoxChanged, this, &PokemonBoxSelectModel::onBoxChange);
  connect(storage, &Storage::boxesFormattedChanged, this, &PokemonBoxSelectModel::onBoxChange);
}

int PokemonBoxSelectModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return 13;
}

QVariant PokemonBoxSelectModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= 13)
    return QVariant();

  // Now return requested information

  // Return the display name decorated accordingly
  if (role == NameRole)
    return getDecoratedName(index.row());

  // Return the value, just the row number
  else if (role == ValueRole)
    return index.row();

  // Return the value, just the row number
  else if (role == IndRole)
    return pairedModel->curBox;

  // Return whether it's disabled or not
  // It's disabled
  // 1) It's the same box as the other side half
  // 2) The boxes are not formatted and the non-party box row is not the
  // current non-party box.
  else if (role == DisabledRole) {
    bool ret = false;

    if((index.row() - 1) == pairedModel->otherModel->curBox)
      ret = true;
    else if(!storage->boxesFormatted && (index.row() - 1) != storage->curBox && index.row() != 0)
      ret = true;

    return ret;
  }

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> PokemonBoxSelectModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "boxInd";
  roles[NameRole] = "boxName";
  roles[ValueRole] = "boxValue";
  roles[DisabledRole] = "boxDisabled";

  return roles;
}

bool PokemonBoxSelectModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if(!index.isValid())
    return false;

  if(index.row() >= 13)
    return false;

  // Now set requested information
  if(role == IndRole) {
    pairedModel->switchBox(value.toInt() - 1);
    return true;
  }

  return false;
}

void PokemonBoxSelectModel::onBoxChange()
{
  // Ask all the data to re-update
  dataChanged(index(0), index(12));
}

QString PokemonBoxSelectModel::getDecoratedName(int box) const
{
  PokemonStorageBox* selBox = (box == 0)
      ? party
      : storage->boxAt(box - 1);

  QString name = boxSelect[box];

  if(selBox->isFull())
    name = boxFullSym + " " + name;
  else if(selBox->pokemon.size() > 0)
    name = boxNotEmptySym + " " + name;
  else
    name = "  " + name;

  if(box > 0 && storage->curBox == (box - 1))
    name += " " + curBoxSym;
  else
    name += "  ";

  return name;
}
