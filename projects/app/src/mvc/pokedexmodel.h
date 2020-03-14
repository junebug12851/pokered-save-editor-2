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
#ifndef POKEDEXMODEL_H
#define POKEDEXMODEL_H

#include <QCollator>
#include <QObject>
#include <QString>
#include <QAbstractListModel>
#include <QVector>

class PlayerPokedex;
class Router;

struct PokedexEntryData {
  PokedexEntryData(QString name, int dex, int id);

  QString name;
  int dex;
  int id;
};

class PokedexModel : public QAbstractListModel
{
  Q_OBJECT

  Q_PROPERTY(int dexSortSelect MEMBER dexSortSelect NOTIFY dexSortSelectChanged)

signals:
  void dexSortSelectChanged();

public:
  enum PokemonStarterRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
    StateRole, // 0 = None, 1 = Seen, 2 = Owned
  };

  enum PokemonSortSelect {
    SortBegin,

    SortDex, // Pokedex Order
    SortName, // Alphabetical Order
    SortInternal, // Internal and Potential Creation Order

    SortEnd
  };

  PokedexModel(PlayerPokedex* pokedex, Router* router);

  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual QHash<int, QByteArray> roleNames() const override;

  void dataChanged(int ind);

  Q_INVOKABLE void dexSortCycle();
  void dexSort();
  void dexSortName();
  void dexSortNum();
  void dexSortInternal();

  void pageClosing();

  int dexSortSelect = SortDex;
  QVector<PokedexEntryData*> dexListCache;
  PlayerPokedex* pokedex = nullptr;
  Router* router = nullptr;

  int dexToListIndex(int ind);
};

#endif // POKEDEXMODEL_H
