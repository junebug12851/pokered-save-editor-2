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

class PlayerPokedex;

/// One item picker row: display name + item index.
struct ItemSelectEntryData {
  ItemSelectEntryData(QString name, int ind);

  QString name; ///< Display name.
  int ind;      ///< Item index.
};

/**
 * @brief Item picker model (select-model variant; see SpeciesSelectModel).
 *
 * Standard picker: cached rows + itemToListIndex() for combo highlighting.
 * Exposed as `brg.itemSelectModel`.
 */
class ItemSelectModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// Picker columns (mapped in roleNames()).
  enum ItemRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
  };

  ItemSelectModel();

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

  QVector<ItemSelectEntryData*> itemListCache; ///< Cached picker rows.

  Q_INVOKABLE int itemToListIndex(int ind); ///< Row index for item @p ind.
};
