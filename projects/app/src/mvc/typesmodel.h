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
#include <QVector>
#include <QObject>
#include <QString>
#include <QAbstractListModel>

/**
 * @brief Type list model -- the elemental types for type pickers.
 *
 * Plain list model (see CreditsModel) over the types DB; valToIndex() maps a type
 * value to its row. Exposed as `brg.typesModel`.
 */
class TypesModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// Columns (mapped in roleNames()).
  enum PokemonStarterRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
  };

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

  Q_INVOKABLE int valToIndex(int val); ///< Row index for type value @p val.
};
