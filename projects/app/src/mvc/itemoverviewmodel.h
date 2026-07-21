/*
  * Copyright 2026 Fairy Fox
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
#include <QVector>
#include <QAbstractListModel>

class ItemStorageBox;

/**
 * @brief Read-only "where are my items" overview for the Bag screen's View All pane.
 *
 * Aggregates the player's two item boxes (bag + PC storage) into one alphabetized
 * table: each distinct item the save holds becomes a row carrying its display name
 * plus the total amount in the bag and the total amount in storage (summed across
 * any duplicate rows -- the editor supports same-item duplicates). Rows with a zero
 * total on BOTH sides are dropped; a side that's zero is surfaced as 0 so the view
 * can hide just that number. Rebuilds (full reset) whenever either box changes, so
 * the pane stays live while the user edits. Exposed as `brg.itemOverviewModel`.
 *
 * @see ItemStorageBox, ItemStorageModel (the editable per-box model).
 */
class ItemOverviewModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// Columns (mapped in roleNames()).
  enum OverviewRoles {
    NameRole = Qt::UserRole + 1, ///< Display name.
    BagCountRole,                ///< Total amount in the bag (0 if none).
    StorageCountRole,            ///< Total amount in PC storage (0 if none).
  };

  /// @param bag the player's bag box; @param storage the PC item box.
  ItemOverviewModel(ItemStorageBox* bag, ItemStorageBox* storage);

  virtual int rowCount(const QModelIndex& parent) const override;           ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

public slots:
  void rebuild(); ///< Re-aggregate both boxes (full model reset). Wired to itemsChanged.

private:
  /// One aggregated item line.
  struct Row {
    QString name;
    int bag;
    int storage;
  };

  ItemStorageBox* bag = nullptr;     ///< The bag box.
  ItemStorageBox* storage = nullptr; ///< The PC item box.
  QVector<Row> rows;                 ///< Current aggregated, sorted rows.
};
