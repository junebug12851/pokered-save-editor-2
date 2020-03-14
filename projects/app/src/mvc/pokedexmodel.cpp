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

#include <pse-db/pokemon.h>
#include "./pokedexmodel.h"
#include "../data/file/expanded/player/playerpokedex.h"
#include "../bridge/router.h"

PokedexEntryData::PokedexEntryData(QString name, int dex, int id)
  : name(name),
    dex(dex),
    id(id)
{}

PokedexModel::PokedexModel(PlayerPokedex* pokedex, Router* router)
  : pokedex(pokedex),
    router(router)
{
  // Populate cache containing dex entries
  for(auto el : PokemonDB::store) {
    if(!el->pokedex)
      continue;

    dexListCache.append(new PokedexEntryData(
                          el->readable,
                          *el->pokedex,
                          el->ind
                          ));
  }

  // Now sort them via Pokedex number
  dexSort();

  // Connect dex changes
  connect(pokedex, &PlayerPokedex::dexItemChanged, this, &PokedexModel::dataChanged);
  connect(this, &PokedexModel::dexSortSelectChanged, this, &PokedexModel::dexSort);

  // When entering the page, it always needs to be in dex order
  connect(this->router, &Router::closeNonModal, this, &PokedexModel::pageClosing);
  connect(this->router, &Router::goHome, this, &PokedexModel::pageClosing);
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
  //auto mon = PokemonDB::ind.value("dex" + QString::number(index.row()), nullptr);
  auto mon = PokemonDB::ind.value("dex" + QString::number(dexListCache.at(index.row())->dex), nullptr);
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
  auto indFixed = dexToListIndex(ind);

  QAbstractListModel::dataChanged(
        index(indFixed),
        index(indFixed)
        );
}

void PokedexModel::dexSortCycle()
{
  dexSortSelect++;
  if(dexSortSelect >= SortEnd)
    dexSortSelect = SortBegin + 1;

  dexSortSelectChanged();
}

void PokedexModel::dexSort()
{
  switch (dexSortSelect) {
  case SortDex:
    dexSortNum();
    break;

  case SortName:
    dexSortName();
    break;

  case SortInternal:
    dexSortInternal();
    break;
  }

  beginResetModel();
  endResetModel();
}

void PokedexModel::dexSortName()
{
  QCollator collator;

  // Setup Collator
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  std::sort(
        dexListCache.begin(),
        dexListCache.end(),
        [&collator](PokedexEntryData* item1, PokedexEntryData* item2)
        {
            return collator.compare(item1->name, item2->name) < 0;
        });
}

void PokedexModel::dexSortNum()
{
  std::sort(
        dexListCache.begin(),
        dexListCache.end(),
        [](PokedexEntryData* item1, PokedexEntryData* item2) {
    return item1->dex < item2->dex;
  });
}

void PokedexModel::dexSortInternal()
{
  std::sort(
        dexListCache.begin(),
        dexListCache.end(),
        [](PokedexEntryData* item1, PokedexEntryData* item2) {
    return item1->id < item2->id;
  });
}

void PokedexModel::pageClosing()
{
  if(dexSortSelect == SortDex)
    return;

  dexSortSelect = SortDex;
  dexSort();
}

int PokedexModel::dexToListIndex(int ind)
{
  int ret = -1;

  for(int i = 0; i < dexListCache.size(); i++) {
    if(ind != dexListCache.at(i)->dex)
      continue;

    ret = i;
    break;
  }

  return ret;
}
