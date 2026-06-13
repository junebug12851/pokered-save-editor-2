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

namespace {
// Render the species name markup the same way the Pokedex / storage grid do, so a
// tooltip name reads "Nidoran ♂" rather than "NIDORAN<m>" (gender markers can show
// up in either case; Mr.Mime gets its space). Presentation only -- a real custom
// nickname won't match these patterns, so it's left untouched.
QString fixSpeciesMarkup(QString s)
{
  s.replace(QStringLiteral("<f>"), QStringLiteral(" ♀"));
  s.replace(QStringLiteral("<F>"), QStringLiteral(" ♀"));
  s.replace(QStringLiteral("<m>"), QStringLiteral(" ♂"));
  s.replace(QStringLiteral("<M>"), QStringLiteral(" ♂"));
  s.replace(QStringLiteral("Mr.Mime"), QStringLiteral("Mr. Mime"));
  return s;
}
}

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
      nicknames.append(fixSpeciesMarkup(mon->nickname));
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

  // Line 2 (ownership): the caught/traded split. Shown ONLY when at least one mon
  // is traded -- if they're all caught (the common case) the split adds nothing, so
  // it's omitted (Twilight). When shown, each side appears only if non-zero, so an
  // all-traded cell reads "×N traded".
  QString line2;
  if(traded > 0) {
    QStringList own;
    if(caught > 0)
      own.append(QStringLiteral("×%1 caught").arg(caught));
    own.append(QStringLiteral("×%1 traded").arg(traded));
    line2 = own.join(QStringLiteral(", "));
  }

  // No nicknames AND nothing traded -> nothing worth a tooltip; leave it empty so
  // the view shows no tooltip at all on that cell.
  if(line1.isEmpty())
    cell.tooltip = line2;
  else if(line2.isEmpty())
    cell.tooltip = line1;
  else
    cell.tooltip = line1 + QStringLiteral("\n") + line2;

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
  // distinct rows, like the items overview keys by index); name + dex resolved once.
  QSet<int> speciesIds;
  QHash<int, QString> names;
  QHash<int, int> dexes;

  for(auto box : colBoxes) {
    if(box == nullptr)
      continue;
    for(auto mon : box->pokemon) {
      if(mon == nullptr)
        continue;
      int id = mon->species;
      speciesIds.insert(id);
      if(!names.contains(id)) {
        names[id] = mon->speciesName();
        dexes[id] = mon->dexNum();
      }
    }
  }

  // ---- Rows: one per species, a cell per column. -------------------------------
  for(int id : speciesIds) {
    Row row;
    row.name = names.value(id, QStringLiteral("???"));
    row.dex = dexes.value(id, 0);
    row.id = id;
    row.cells.reserve(colBoxes.size());
    for(auto box : colBoxes)
      row.cells.append(buildCell(box, id));
    rows.append(row);
  }

  applySort();

  endResetModel();
  columnsChanged();
}

void PokemonOverviewModel::applySort()
{
  // The Pokedex screen's sort orders (PokedexModel): dex number, alphabetical,
  // internal id. Stable, deterministic ties (id) so the order never wobbles.
  if(sortSelect == SortName) {
    QCollator collator;
    collator.setNumericMode(true);
    collator.setIgnorePunctuation(true);
    std::sort(rows.begin(), rows.end(), [&collator](const Row& a, const Row& b) {
      int c = collator.compare(a.name, b.name);
      return (c != 0) ? (c < 0) : (a.id < b.id);
    });
  }
  else if(sortSelect == SortInternal) {
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
      return a.id < b.id;
    });
  }
  else { // SortDex (default fallback)
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
      return (a.dex != b.dex) ? (a.dex < b.dex) : (a.id < b.id);
    });
  }
}

void PokemonOverviewModel::sortCycle()
{
  // Mirror PokedexModel::dexSortCycle -- advance through the three orders, wrapping
  // past the sentinels.
  sortSelect++;
  if(sortSelect >= SortEnd)
    sortSelect = SortBegin + 1;

  beginResetModel();
  applySort();
  endResetModel();

  sortSelectChanged();
}

QString PokemonOverviewModel::sortLabel() const
{
  switch(sortSelect) {
  case SortDex:      return QStringLiteral("Dex order");
  case SortName:     return QStringLiteral("Alphabetical");
  case SortInternal: return QStringLiteral("Internal order");
  default:           return QString();
  }
}

QString PokemonOverviewModel::sortIcon() const
{
  switch(sortSelect) {
  case SortDex:      return QStringLiteral("qrc:/assets/icons/sort/pokedex.png");
  case SortName:     return QStringLiteral("qrc:/assets/icons/sort/alphabetical.png");
  case SortInternal: return QStringLiteral("qrc:/assets/icons/sort/internal.png");
  default:           return QString();
  }
}
