/*
  * Copyright 2020 Twilight
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
#include <QAbstractListModel>

class PokemonStorageModel;
class Storage;
class PlayerPokemon;

/**
 * @brief The "which PC box" selector model (Party + 12 boxes).
 *
 * A combo-box model listing the party and the twelve storage boxes, each decorated
 * with a fill-status symbol (empty/partly/full + a current-box marker). It is
 * @e paired to a PokemonStorageModel: selecting a box here drives that model's
 * current box, and vice versa (see the note at @ref pairedModel). setData() handles
 * the selection write-back. Exposed as `brg.pokemonBoxSelectModel1/2`.
 *
 * @see PokemonStorageModel (the paired model), Storage, PlayerPokemon.
 */
class PokemonBoxSelectModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// Columns (mapped in roleNames()).
  enum PokemonBoxSelectModelRoles {
    NameRole = Qt::UserRole + 1,
    ValueRole,
    DisabledRole,
    IndRole,
  };

  PokemonBoxSelectModel(PokemonStorageModel* pairedModel); ///< @param pairedModel the storage model to drive.

  QString boxEmptySym = " ";    ///< Decoration: empty box.
  QString boxNotEmptySym = "○"; ///< Decoration: partly-filled box.
  QString boxFullSym = "●";     ///< Decoration: full box.
  QString curBoxSym = "◁";      ///< Decoration: current box marker.

  QString boxSelect[13] = {
    "Party",
    "Storage Box 1",
    "Storage Box 2",
    "Storage Box 3",
    "Storage Box 4",
    "Storage Box 5",
    "Storage Box 6",
    "Storage Box 7",
    "Storage Box 8",
    "Storage Box 9",
    "Storage Box 10",
    "Storage Box 11",
    "Storage Box 12",
  }; ///< Row labels: Party plus the 12 boxes.

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count (13).
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override; ///< Selection write-back.

  Q_INVOKABLE void onBoxChange(); ///< React to this selector changing.
  void onPairedBoxChange();       ///< React to the paired model's box changing.
  QString getDecoratedName(int box) const; ///< Row label with its fill-status symbol.

  // This needs to be paired to a PokemonStorageModel, it interacts with the
  // model and controls it's current box. If the model updates its own box
  // this updates as well
  PokemonStorageModel* pairedModel = nullptr; ///< The storage model this selector drives (see note).
  Storage* storage = nullptr;                 ///< The PC storage (for fill status).
  PlayerPokemon* party = nullptr;             ///< The party (box 0).
};
