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
#include "./pokemonstartersmodel.h"

int PokemonStartersModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return 3;
}

QVariant PokemonStartersModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= 3)
    return QVariant();

  // Get Pokemon and ensure it's valid to prevent crashing
  auto mon = getMon(index.row());
  if(mon == nullptr)
    return QVariant();

  // Now return requested information
  if (role == IndRole)
    return mon->ind;
  else if (role == NameRole) {
    if(mon->readable != "")
      return mon->readable;

    return mon->name;
  }

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> PokemonStartersModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "monInd";
  roles[NameRole] = "monName";

  return roles;
}

int PokemonStartersModel::valToIndex(int val)
{
  auto mon = PokemonDB::ind.value(QString::number(val), nullptr);
  if(mon == nullptr)
    return 3;

  if(mon->readable == "Bulbasaur")
    return 0;
  else if(mon->readable == "Charmander")
    return 1;
  else if(mon->readable == "Squirtle")
    return 2;

  return 3;
}

PokemonDBEntry* PokemonStartersModel::getMon(int ind) const
{
  return PokemonDB::ind.value(starters[ind], nullptr);
}
