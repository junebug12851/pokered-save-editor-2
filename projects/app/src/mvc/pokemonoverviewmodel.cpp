/*
  * Copyright 2026 Twilight
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
 * @file pokemonoverviewmodel.cpp
 * @brief Implementation of PokemonOverviewModel. See pokemonoverviewmodel.h.
 */

#include <algorithm>
#include <QHash>
#include <QSet>
#include <QCollator>
#include <QVariantList>

#include "./pokemonoverviewmodel.h"
#include <pse-savefile/expanded/storage.h>
#include <pse-savefile/expanded/fragments/pokemonstoragebox.h>
#include <pse-savefile/expanded/fragments/pokemonbox.h>
#include <pse-savefile/expanded/player/playerbasics.h>

PokemonOverviewModel::PokemonOverviewModel(PokemonStorageBox* party, Storage* storage, PlayerBasics* basics)
  : party(party),
    storage(storage),
    basics(basics)
{
  // Stay live: any add/remove/move on the party or any box re-aggregates. The box
  // pointers are stable for the save's lifetime, so connect once here. (Species /
  // nickname / OT edits happen in the detail editor and don't emit pokemonChanged
  // on the box -- the QML pane rebuilds the table on open to cover those.)
  if(this->party != nullptr)
    connect(this->party, &PokemonStorageBox::pokemonChanged, this, &PokemonOverviewModel::rebuild);

  if(this->storage != nullptr) {
    for(int i = 0; i < storage->boxCount(); ++i) {
      auto box = storage->boxAt(i);
      if(box != nullptr)
        connect(box, &PokemonStorageBox::pokemonChanged, this, &PokemonOverviewModel::rebuild);
    }
  }

  rebuild();
}

int PokemonOverviewModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent)
  return rows.size();
}

QStringList PokemonOverviewModel::columns() const
{
  return colLabels;
}

QVariant PokemonOverviewModel::data(const QModelIndex& index, int role) const
{
  if(!index.isValid() || index.row() < 0 || index.row() >= rows.size())
    return QVariant();

  const Row& r = rows.at(index.row());

  if(role == NameRole)
    return r.name;

  if(role == CountsRole) {
    QVariantList out;
    out.reserve(r.cells.size());
    for(const Cell& c : r.cells)
      out.append(c.count);
    return out;
  }

  if(role == TooltipsRole) {
    QVariantList out;
    out.reserve(r.cells.size());
    for(const Cell& c : r.cells)
      out.append(c.tooltip);
    return out;
  }

  return QVariant();
}

QHash<int, QByteArray> PokemonOverviewModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[NameRole] = "speciesName";
  roles[CountsRole] = "counts";
  roles[TooltipsRole] = "tooltips";
  return roles;
}

PokemonOverviewModel::Cell PokemonOverviewModel::buildCell(PokemonStorageBox* box, int speciesId) const
{
  Cell cell;
  if(box == nullptr)
    return cell;

  // Gather every mon of this species in this box, tallying caught vs traded and
  // collecting the distinct nicknames (mons whose nickname differs from the
  // species name -- PokemonBox::hasNickname).
  QStringList nicknames;
  int others = 0;
  int caught = 0;
  int traded = 0;

  for(auto mon : box->pokemon) {
    if(mon == nullptr || mon->species != speciesId)
      continue;

    cell.count++;

    if(mon->hasTradeStatus(basics))
      traded++;
    else
      caught++;

    if(mon->hasNickname())
      nicknames.append(mon->nickname);
    else
      others++;
  }

  if(cell.count == 0)
    return cell;

  // ---- Build the tooltip -------------------------------------------------------
  // Line 1 (names): the differing nicknames spelled out, then an "...and xN
  // other(s)" tail for the un-nicknamed remainder. When NOTHING is nicknamed the
  // line is omitted (the cell already shows the count, and line 2 splits it).
  QString line1;
  if(!nicknames.isEmpty()) {
    line1 = nicknames.join(QStringLiteral(", "));
    if(others > 0)
      line1 += QStringLiteral(", and ×%1 %2")
                 .arg(others)
                 .arg(others == 1 ? QStringLiteral("other") : QStringLiteral("others"));
  }

  // Line 2 (ownership): caught/traded split, hiding whichever side is zero (the
  // whole screen hides zeros).
  QStringList own;
  if(caught > 0)
    own.append(QStringLiteral("×%1 caught").arg(caught));
  if(traded > 0)
    own.append(QStringLiteral("×%1 traded").arg(traded));
  QString line2 = own.join(QStringLiteral(", "));

  cell.tooltip = line1.isEmpty() ? line2 : (line1 + QStringLiteral("\n") + line2);
  return cell;
}

void PokemonOverviewModel::rebuild()
{
  beginResetModel();
  rows.clear();
  colBoxes.clear();
  colLabels.clear();

  // ---- Columns: the party first, then ONLY the boxes that hold Pokemon. --------
  colBoxes.append(party);
  colLabels.append(QStringLiteral("Party"));

  if(storage != nullptr) {
    for(int i = 0; i < storage->boxCount(); ++i) {
      auto box = storage->boxAt(i);
      if(box != nullptr && box->pokemonCount() > 0) {
        colBoxes.append(box);
        colLabels.append(QStringLiteral("Box %1").arg(i + 1));
      }
    }
  }

  // ---- Species universe: every distinct species id present in any column. ------
  // Keyed by raw species id (so two ids that happen to share a display name stay
  // distinct rows, like the items overview keys by index); name resolved once.
  QSet<int> speciesIds;
  QHash<int, QString> names;

  for(auto box : colBoxes) {
    if(box == nullptr)
      continue;
    for(auto mon : box->pokemon) {
      if(mon == nullptr)
        continue;
      int id = mon->species;
      speciesIds.insert(id);
      if(!names.contains(id))
        names[id] = mon->speciesName();
    }
  }

  // ---- Rows: one per species, a cell per column. -------------------------------
  for(int id : speciesIds) {
    Row row;
    row.name = names.value(id, QStringLiteral("???"));
    row.cells.reserve(colBoxes.size());
    for(auto box : colBoxes)
      row.cells.append(buildCell(box, id));
    rows.append(row);
  }

  // Alphabetical by species name (numeric-aware, punctuation-insensitive), mirroring
  // the items overview so the table reads consistently regardless of box order.
  QCollator collator;
  collator.setNumericMode(true);
  collator.setIgnorePunctuation(true);
  std::sort(rows.begin(), rows.end(), [&collator](const Row& a, const Row& b) {
    return collator.compare(a.name, b.name) < 0;
  });

  endResetModel();
  columnsChanged();
}
