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

#include "../data/db/pokemon.h"
#include "./pokedexmodel.h"
#include "../data/file/expanded/player/playerpokedex.h"

PokedexModel::PokedexModel(PlayerPokedex* pokedex)
  : pokedex(pokedex)
{
  connect(pokedex, &PlayerPokedex::dexItemChanged, this, &PokedexModel::dataChanged);
}

int PokedexModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return pokemonDexCount;
}

QVariant PokedexModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= pokemonDexCount)
    return QVariant();

  // Get Pokemon and return if invalid
  auto mon = PokemonDB::ind.value("dex" + QString::number(index.row()), nullptr);
  if(mon == nullptr)
    return QVariant();

  // Now return requested information
  if (role == IndRole)
    return *mon->pokedex;
  else if (role == NameRole) {
    if(mon->readable != "")
      return mon->readable;

    return mon->name;
  }
  else if(role == StateRole)
    return pokedex->getState(*mon->pokedex);

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> PokedexModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "dexInd";
  roles[NameRole] = "dexName";
  roles[StateRole] = "dexState";

  return roles;
}

void PokedexModel::dataChanged(int ind)
{
  QAbstractListModel::dataChanged(
        index(ind),
        index(ind)
        );
  //  beginResetModel();
  //  endResetModel();
}
