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
#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QCollator>
#include <QAbstractListModel>

class Storage;
class PokemonStorageBox;
class PlayerBasics;

/**
 * @brief Read-only "where are my Pokemon" overview for the Pokemon screen's View
 *        All pane -- the Pokemon analogue of ItemOverviewModel.
 *
 * Aggregates the party and the 12 PC boxes into one alphabetized species table.
 * Each row is a distinct species (keyed by raw species id, displayed by species
 * name -- NOT nickname); the columns are the **party first, then only the boxes
 * that actually hold Pokemon** (empty boxes are omitted entirely, keeping the
 * table clean). Each cell carries the count of that species in that column plus a
 * precomputed hover tooltip listing the differing nicknames in that cell, an
 * "...and xN others" tail for the un-nicknamed remainder, and a caught/traded
 * breakdown (a mon is "traded" when its OT name/ID differs from the player's --
 * PokemonBox::hasTradeStatus). A zero cell carries an empty tooltip so the view
 * can hide just that number. Rebuilds (full reset) whenever the party or any box
 * changes, plus an explicit rebuild-on-open. Exposed as `brg.pokemonOverviewModel`.
 *
 * The column headers are exposed separately as the `columns` string list so the
 * QML header row and the per-row count cells stay aligned.
 *
 * @see ItemOverviewModel (the items analogue), PokemonStorageBox, Storage,
 *      PokemonBox::hasTradeStatus / hasNickname / speciesName.
 */
class PokemonOverviewModel : public QAbstractListModel
{
  Q_OBJECT

  /// Column header labels, in display order (e.g. "Party", "Box 1", ...). One per
  /// entry in each row's `counts`/`tooltips`. Changes on every rebuild.
  Q_PROPERTY(QStringList columns READ columns NOTIFY columnsChanged)

  /// Current species-column sort order (see SortSelect). Cycled by sortCycle().
  Q_PROPERTY(int sortSelect MEMBER sortSelect NOTIFY sortSelectChanged)
  /// Human-readable label for the current sort (for the header sort-icon tooltip).
  Q_PROPERTY(QString sortLabel READ sortLabel NOTIFY sortSelectChanged)

public:
  /// Roles (mapped in roleNames()).
  enum OverviewRoles {
    NameRole = Qt::UserRole + 1, ///< Species display name.
    CountsRole,                  ///< Per-column counts (QVariantList<int>, aligned to `columns`).
    TooltipsRole,                ///< Per-column hover tooltips (QVariantList<QString>; empty when count 0).
  };

  /// Species-column sort orders -- the SAME set the Pokedex screen cycles
  /// (PokedexModel::PokemonSortSelect): dex number, alphabetical, internal id.
  enum SortSelect {
    SortBegin,
    SortDex,      ///< Pokedex number order.
    SortName,     ///< Alphabetical by species name.
    SortInternal, ///< Internal / creation (raw species id) order.
    SortEnd
  };

  /// @param party the player's party; @param storage the PC (12 boxes);
  /// @param basics the player's basics (for trade-status comparison).
  PokemonOverviewModel(PokemonStorageBox* party, Storage* storage, PlayerBasics* basics);

  virtual int rowCount(const QModelIndex& parent) const override;           ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

  QStringList columns() const; ///< @see columns property.
  QString sortLabel() const;   ///< @see sortLabel property.

  Q_INVOKABLE void sortCycle(); ///< Advance to the next species sort order (re-sorts in place).

public slots:
  void rebuild(); ///< Re-aggregate the party + boxes (full model reset). Wired to pokemonChanged.

signals:
  void columnsChanged();
  void sortSelectChanged();

private:
  /// One table cell: how many of the row's species sit in the column's box, and
  /// the precomputed hover tooltip describing them.
  struct Cell {
    int count = 0;
    QString tooltip;
  };

  /// One aggregated species line: display name + sort keys + one Cell per column.
  struct Row {
    QString name; ///< Species display name (raw readable, e.g. "Nidoran<f>").
    int dex;      ///< Pokedex number (for SortDex).
    int id;       ///< Internal species id (for SortInternal / tiebreak).
    QVector<Cell> cells;
  };

  /// Build the tooltip for the @p speciesId mons inside @p box.
  Cell buildCell(PokemonStorageBox* box, int speciesId) const;

  void applySort(); ///< Order `rows` by the current sortSelect (in place, no model signals).

  PokemonStorageBox* party = nullptr; ///< The party (always the first column).
  Storage* storage = nullptr;         ///< The PC (its 12 boxes supply the rest).
  PlayerBasics* basics = nullptr;     ///< Player identity, for trade detection.

  QVector<PokemonStorageBox*> colBoxes; ///< The boxes backing each column (party + non-empty boxes).
  QStringList colLabels;                ///< The header label for each column.
  QVector<Row> rows;                    ///< Current aggregated, sorted rows.

  int sortSelect = SortName; ///< @see sortSelect property (default alphabetical).
};
