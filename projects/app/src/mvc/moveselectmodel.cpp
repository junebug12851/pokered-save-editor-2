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

#include <pse-db/moves.h>
#include <pse-db/pokemon.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include "./moveselectmodel.h"

MoveSelectEntry::MoveSelectEntry(QString name, int ind)
  : name(name),
    ind(ind)
{}

MoveSelectModel::MoveSelectModel()
{
  connect(this, &MoveSelectModel::monChanged, this, &MoveSelectModel::onMonChange);
  onMonChange();
}

int MoveSelectModel::rowCount(const QModelIndex& parent) const
{
  // Not a tree, just a list, there's no parent
  Q_UNUSED(parent)

  // Return list count
  return moveListCache.size();
}

QVariant MoveSelectModel::data(const QModelIndex& index, int role) const
{
  // If index is invalid in any way, return nothing
  if (!index.isValid())
    return QVariant();

  if (index.row() >= moveListCache.size())
    return QVariant();

  // Get Item from Item List Cache
  auto item = moveListCache.at(index.row());

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

QHash<int, QByteArray> MoveSelectModel::roleNames() const
{
  QHash<int, QByteArray> roles;

  roles[IndRole] = "moveInd";
  roles[NameRole] = "moveName";

  return roles;
}

int MoveSelectModel::moveToListIndex(int ind)
{
  int ret = -1;

  for(int i = 0; i < moveListCache.size(); i++) {
    if(ind != moveListCache.at(i)->ind)
      continue;

    ret = i;
    break;
  }

  return ret;
}

void MoveSelectModel::onMonChange()
{
  for(auto el : moveListCache)
    delete el;

  moveListCache.clear();

  if(mon == nullptr)
    rebuildListGeneral();
  else
    rebuildListSpecific();

  beginResetModel();
  endResetModel();
}

void MoveSelectModel::rebuildListGeneral()
{
  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  moveListCache.append(new MoveSelectEntry("", 0));

  /////////////////////////
  ///////// Normal Moves
  /////////////////////////

  moveListCache.append(new MoveSelectEntry("--- Normal Moves ---", -1));

  // Gather normal repeatable items and sort by name, then add into list
  QVector<MoveDBEntry*> tmp;

  for(auto el : MovesDB::store) {
    if(!el->glitch && !el->hm)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveDBEntry* mon1, const MoveDBEntry* mon2)
      {
          return collator.compare(mon1->readable, mon2->readable) < 0;
      });

  for(auto el : tmp) {
    moveListCache.append(new MoveSelectEntry(el->readable, el->ind));
  }

  tmp.clear();

  /////////////////////////
  ///////// HM Moves
  /////////////////////////

  moveListCache.append(new MoveSelectEntry("--- HM Moves ---", -1));

  for(auto el : MovesDB::store) {
    if(!el->glitch && el->hm)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveDBEntry* mon1, const MoveDBEntry* mon2)
      {
          return collator.compare(
                "HM 0" + QString::number(*mon1->hm) + ": " + mon1->readable,
                "HM 0" + QString::number(*mon2->hm) + ": " + mon2->readable) < 0;
      });

  for(auto el : tmp) {
    moveListCache.append(new MoveSelectEntry(
                           "HM 0" + QString::number(*el->hm) + ": " + el->readable,
                           el->ind));
  }

  tmp.clear();

  /////////////////////////
  ///////// Glitch Moves
  /////////////////////////

  // Add first category
  moveListCache.append(new MoveSelectEntry("--- Glitch Moves ---", -1));

  for(auto el : MovesDB::store) {
    if(el->glitch)
      tmp.append(el);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveDBEntry* mon1, const MoveDBEntry* mon2)
      {
          return collator.compare(mon1->readable, mon2->readable) < 0;
      });

  for(auto el : tmp) {
    moveListCache.append(new MoveSelectEntry(el->readable, el->ind));
  }

  tmp.clear();
}

void MoveSelectModel::rebuildListSpecific()
{
  if(mon == nullptr)
    rebuildListGeneral();

  moveListCache.append(new MoveSelectEntry("", 0));

  // Setup Collator
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);

  /////////////////////////
  ///////// Initial Moves
  /////////////////////////

  moveListCache.append(new MoveSelectEntry("--- Initial Moves ---", -1));

  QVector<MoveSelectEntry*> tmp;
  QVector<MoveDBEntry*> usedMoves;

  for(auto monMove : mon->toInitial) {
    if(usedMoves.contains(monMove))
      continue;

    tmp.append(new MoveSelectEntry(monMove->readable, monMove->ind));
    usedMoves.append(monMove);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveSelectEntry* mon1, const MoveSelectEntry* mon2)
      {
          return collator.compare(mon1->name, mon2->name) < 0;
      });

  if(tmp.size() == 0)
    moveListCache.removeLast();

  for(auto el : tmp) {
    moveListCache.append(el);
  }

  tmp.clear();

  /////////////////////////
  ///////// Normal Moves
  /////////////////////////

  moveListCache.append(new MoveSelectEntry("--- Learnable Moves ---", -1));

  for(auto monMove : mon->moves) {
    if(usedMoves.contains(monMove->toMove))
      continue;

    tmp.append(new MoveSelectEntry("Lv " + QString::number(monMove->level) + ": " + monMove->toMove->readable,
                                   monMove->toMove->ind));
    usedMoves.append(monMove->toMove);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveSelectEntry* mon1, const MoveSelectEntry* mon2)
      {
          return collator.compare(mon1->name, mon2->name) < 0;
      });

  if(tmp.size() == 0)
    moveListCache.removeLast();

  for(auto el : tmp) {
    moveListCache.append(el);
  }

  tmp.clear();

  /////////////////////////
  ///////// TM/HM Moves
  /////////////////////////

  moveListCache.append(new MoveSelectEntry("--- Teachable Moves ---", -1));

  for(auto monMove : mon->toTmHmMove) {
    if(usedMoves.contains(monMove) || monMove->hm)
      continue;

    QString prefix;
    if((*monMove->tm) < 10)
      prefix += "TM 0" + QString::number(*monMove->tm) + ": ";
    else
      prefix += "TM " + QString::number(*monMove->tm) + ": ";

    tmp.append(new MoveSelectEntry(prefix + monMove->readable,
                                   monMove->ind));
    usedMoves.append(monMove);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveSelectEntry* mon1, const MoveSelectEntry* mon2)
      {
          return collator.compare(mon1->name, mon2->name) < 0;
      });

  bool tmEmpty = false;
  if(tmp.size() == 0)
    tmEmpty = true;

  for(auto el : tmp) {
    moveListCache.append(el);
  }

  tmp.clear();

  for(auto monMove : mon->toTmHmMove) {
    if(usedMoves.contains(monMove) || !monMove->hm)
      continue;

    QString prefix;
    if(monMove->hm)
      prefix += "HM 0" + QString::number(*monMove->hm) + ": ";

    tmp.append(new MoveSelectEntry(prefix + monMove->readable,
                                   monMove->ind));
    usedMoves.append(monMove);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveSelectEntry* mon1, const MoveSelectEntry* mon2)
      {
          return collator.compare(mon1->name, mon2->name) < 0;
      });

  if(tmp.size() == 0 && tmEmpty)
    moveListCache.removeLast();

  for(auto el : tmp) {
    moveListCache.append(el);
  }

  tmp.clear();

  /////////////////////////
  ///////// Other Moves
  /////////////////////////

  moveListCache.append(new MoveSelectEntry("--- Incompatible Moves ---", -1));

  for(auto monMove : MovesDB::store) {
    if(usedMoves.contains(monMove) || monMove->glitch || monMove->hm)
      continue;

    tmp.append(new MoveSelectEntry(monMove->readable,
                                   monMove->ind));
    usedMoves.append(monMove);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveSelectEntry* mon1, const MoveSelectEntry* mon2)
      {
          return collator.compare(mon1->name, mon2->name) < 0;
      });

  bool otherMovesEmpty = false;
  if(tmp.size() == 0)
    otherMovesEmpty = true;

  for(auto el : tmp) {
    moveListCache.append(el);
  }

  tmp.clear();

  for(auto monMove : MovesDB::store) {
    if(usedMoves.contains(monMove) || monMove->glitch || !monMove->hm)
      continue;

    QString prefix;
    if(monMove->hm)
      prefix += "HM 0" + QString::number(*monMove->hm) + ": ";

    tmp.append(new MoveSelectEntry(prefix + monMove->readable,
                                   monMove->ind));
    usedMoves.append(monMove);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveSelectEntry* mon1, const MoveSelectEntry* mon2)
      {
          return collator.compare(mon1->name, mon2->name) < 0;
      });

  if(tmp.size() == 0 && otherMovesEmpty)
    moveListCache.removeLast();

  for(auto el : tmp) {
    moveListCache.append(el);
  }

  tmp.clear();

  /////////////////////////
  ///////// Glitch Moves
  /////////////////////////

  moveListCache.append(new MoveSelectEntry("--- Glitch Moves ---", -1));

  for(auto monMove : MovesDB::store) {
    if(usedMoves.contains(monMove) || !monMove->glitch)
      continue;

    tmp.append(new MoveSelectEntry(monMove->readable,
                                   monMove->ind));
    usedMoves.append(monMove);
  }

  std::sort(
      tmp.begin(),
      tmp.end(),
      [&collator](const MoveSelectEntry* mon1, const MoveSelectEntry* mon2)
      {
          return collator.compare(mon1->name, mon2->name) < 0;
      });

  if(tmp.size() == 0)
    moveListCache.removeLast();

  for(auto el : tmp) {
    moveListCache.append(el);
  }

  tmp.clear();
}

void MoveSelectModel::monFromBox(PokemonBox* box)
{
  if(box == nullptr ||
     box->toData() == nullptr ||
     !box->toData()->pokedex) {
    mon = nullptr;
    monChanged();
    return;
  }

  mon = box->toData();
  monChanged();
}
