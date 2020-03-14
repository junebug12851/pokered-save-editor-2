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
#ifndef POKEMONSTARTERSMODEL_H
#define POKEMONSTARTERSMODEL_H

#include <QObject>
#include <QString>
#include <QAbstractListModel>

class PokemonDBEntry;

class PokemonStartersModel : public QAbstractListModel
{
  Q_OBJECT

public:
  enum PokemonStarterRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
  };

  // 3 Core Starters
  QString starters[3] = {
    "Bulbasaur",
    "Charmander",
    "Squirtle"
  };

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE int valToIndex(int val);
  PokemonDBEntry* getMon(int ind) const;
};

#endif // POKEMONSTARTERSMODEL_H
