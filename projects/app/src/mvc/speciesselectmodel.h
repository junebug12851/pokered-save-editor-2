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
#include <QVector>

/// One species picker row: display name + species index.
struct SpeciesSelectEntry {
  SpeciesSelectEntry(QString name, int ind);

  QString name; ///< Display name.
  int ind;      ///< Species index.
};

/**
 * @brief A species picker list model (the "select model" variant).
 *
 * The picker flavour of the mvc-model convention (see CreditsModel): besides the
 * standard rowCount/data/roleNames, it holds a @ref speciesListCache of
 * SpeciesSelectEntry rows and adds speciesToListIndex() to map a species index to
 * its row -- so a combo box can highlight the current value. The other `*SelectModel`
 * pickers (status/nature/move/map/item/box) follow the same shape.
 *
 * @see PokemonDB (the source), Bridge (exposes this as `brg.speciesSelectModel`).
 */
class SpeciesSelectModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// Picker columns (mapped to names in roleNames()).
  enum ItemRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
  };

  SpeciesSelectModel(); ///< Build the cache from the species DB.

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Number of species rows.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Value for a row + role.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name map.

  QVector<SpeciesSelectEntry*> speciesListCache; ///< Cached picker rows.

  Q_INVOKABLE int speciesToListIndex(int ind); ///< Row index for species @p ind (for combo highlighting).
};
