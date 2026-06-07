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
#include <QCollator>
#include <QObject>
#include <QString>
#include <QAbstractListModel>
#include <QVector>

class PlayerPokedex;
class Router;

/// One Pokedex grid row: name, dex number, internal id.
struct PokedexEntryData {
  PokedexEntryData(QString name, int dex, int id);

  QString name; ///< Species name.
  int dex;      ///< Pokedex number.
  int id;       ///< Internal id.
};

/**
 * @brief The Pokedex grid model -- seen/owned state, with cycling sort orders.
 *
 * Wraps the save's PlayerPokedex for the dex screen. Each row carries a tri-state
 * @ref StateRole (none/seen/owned). @ref dexSortSelect cycles through dex/name/
 * internal order via dexSortCycle(). Holds the @ref router to react to page open/
 * close. Exposed as `brg.pokedexModel`.
 */
class PokedexModel : public QAbstractListModel
{
  Q_OBJECT

  Q_PROPERTY(int dexSortSelect MEMBER dexSortSelect NOTIFY dexSortSelectChanged) ///< Current sort order.

signals:
  void dexSortSelectChanged();

public:
  /// Columns (mapped in roleNames()).
  enum PokemonStarterRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
    StateRole, // 0 = None, 1 = Seen, 2 = Owned
  };

  /// The available sort orders (cycled by dexSortCycle()).
  enum PokemonSortSelect {
    SortBegin,

    SortDex, // Pokedex Order
    SortName, // Alphabetical Order
    SortInternal, // Internal and Potential Creation Order

    SortEnd
  };

  PokedexModel(PlayerPokedex* pokedex, Router* router);

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

  void dataChanged(int ind); ///< Notify that dex entry @p ind changed.

  Q_INVOKABLE void dexSortCycle(); ///< Advance to the next sort order.
  void dexSort();         ///< Apply the current sort.
  void dexSortName();     ///< Sort alphabetically.
  void dexSortNum();      ///< Sort by dex number.
  void dexSortInternal(); ///< Sort by internal id.

  void pageClosing(); ///< Hook for when the dex page closes.

  int dexSortSelect = SortDex;             ///< @see dexSortSelect property.
  QVector<PokedexEntryData*> dexListCache; ///< Cached, currently-sorted rows.
  PlayerPokedex* pokedex = nullptr;        ///< The save's dex.
  Router* router = nullptr;                ///< For page open/close hooks.

  int dexToListIndex(int ind); ///< Row index for species @p ind under the current sort.
};
