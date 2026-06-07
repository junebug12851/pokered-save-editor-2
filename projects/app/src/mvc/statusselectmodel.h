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

/// One status-condition picker row: display name + status index.
struct StatusSelectEntry {
  StatusSelectEntry(QString name, int ind);

  QString name; ///< Display name.
  int ind;      ///< Status index.
};

/**
 * @brief Status-condition picker model (select-model variant; see SpeciesSelectModel).
 *
 * Standard picker: cached rows + statusToListIndex() for combo highlighting.
 * Exposed as `brg.statusSelectModel`.
 */
class StatusSelectModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// Picker columns (mapped in roleNames()).
  enum ItemRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
  };

  StatusSelectModel();

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

  QVector<StatusSelectEntry*> statusListCache; ///< Cached picker rows.

  Q_INVOKABLE int statusToListIndex(int ind); ///< Row index for status @p ind.
};
