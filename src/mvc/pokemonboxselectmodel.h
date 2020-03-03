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
#ifndef POKEMONBOXSELECTMODEL_H
#define POKEMONBOXSELECTMODEL_H

#include <QObject>
#include <QString>
#include <QAbstractListModel>

class PokemonStorageModel;
class Storage;
class PlayerPokemon;

class PokemonBoxSelectModel : public QAbstractListModel
{
  Q_OBJECT

public:
  enum PokemonBoxSelectModelRoles {
    NameRole = Qt::UserRole + 1,
    ValueRole,
    DisabledRole,
    IndRole,
  };

  PokemonBoxSelectModel(PokemonStorageModel* pairedModel);

  QString boxEmptySym = " ";
  QString boxNotEmptySym = "○";
  QString boxFullSym = "●";
  QString curBoxSym = "◁";

  QString boxSelect[13] = {
    "Party",
    "Box 1",
    "Box 2",
    "Box 3",
    "Box 4",
    "Box 5",
    "Box 6",
    "Box 7",
    "Box 8",
    "Box 9",
    "Box 10",
    "Box 11",
    "Box 12",
  };

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;

  void onBoxChange();
  void onPairedBoxChange();
  QString getDecoratedName(int box) const;

  // This needs to be paired to a PokemonStorageModel, it interacts with the
  // model and controls it's current box. If the model updates its own box
  // this updates as well
  PokemonStorageModel* pairedModel = nullptr;
  Storage* storage = nullptr;
  PlayerPokemon* party = nullptr;
};

#endif // POKEMONBOXSELECTMODEL_H
