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

#include "../data/db/pokemon.h"
#include "./speciesselectmodel.h"

SpeciesSelectEntry::SpeciesSelectEntry(QString name, int ind)
  : name(name),
    ind(ind)
{}

SpeciesSelectModel::SpeciesSelectModel()
{
  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  // Add first category
  speciesListCache.append(new SpeciesSelectEntry("--- Pokedex Pokemon ---", -1));

  // Gather normal repeatable items and sort by name, then add into list
  QVector<PokemonDBEntry*> tmp;

  for(auto el : PokemonDB::store) {
    if(el->pokedex)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const PokemonDBEntry* mon1, const PokemonDBEntry* mon2)
      {
          return collator.compare(mon1->readable, mon2->readable) < 0;
      });

  for(auto el : tmp) {
    speciesListCache.append(new SpeciesSelectEntry(el->readable, el->ind));
  }

  tmp.clear();

  // Add 2nd category
  speciesListCache.append(new SpeciesSelectEntry("--- Glitch Pokemon ---", -1));

  for(auto el : PokemonDB::store) {
    if(!el->pokedex)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const PokemonDBEntry* mon1, const PokemonDBEntry* mon2)
      {
          return collator.compare(mon1->readable, mon2->readable) < 0;
      });

  for(auto el : tmp) {
    speciesListCache.append(new SpeciesSelectEntry(el->readable, el->ind));
  }

  tmp.clear();
}

int SpeciesSelectModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return speciesListCache.size();
}

QVariant SpeciesSelectModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= speciesListCache.size())
    return QVariant();

  // Get Item from Item List Cache
  auto item = speciesListCache.at(index.row());

  if(item == nullptr)
    return QVariant();

  // Now return requested information
  if (role == IndRole)
    return item->ind;
  else if (role == NameRole)
    return item->name;

  // All else fails, return nothing
  return QVariant();
}

QHash<int, QByteArray> SpeciesSelectModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "speciesInd";
  roles[NameRole] = "speciesName";

  return roles;
}

int SpeciesSelectModel::speciesToListIndex(int ind)
{
  int ret = -1;

  for(int i = 0; i < speciesListCache.size(); i++) {
    if(ind != speciesListCache.at(i)->ind)
      continue;

    ret = i;
    break;
  }

  return ret;
}
