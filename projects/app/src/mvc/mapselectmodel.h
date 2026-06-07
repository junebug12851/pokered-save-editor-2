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

class AreaMap;

/// One map picker row: display name + map index.
struct MapSelectEntry {
  MapSelectEntry(QString name, int ind);

  QString name; ///< Display name.
  int ind;      ///< Map index.
};

/**
 * @brief Map picker model, tied to the current AreaMap.
 *
 * Select-model variant (see SpeciesSelectModel) holding the live @ref map so
 * rebuild() can refresh against it; mapToListIndex() highlights the current map.
 * Backs the "place player on a map" picker. Exposed as `brg.mapSelectModel`.
 */
class MapSelectModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// Picker columns (mapped in roleNames()).
  enum ItemRoles {
    IndRole = Qt::UserRole + 1,
    NameRole,
  };

  MapSelectModel(AreaMap* map); ///< @param map the live area map.

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Row count.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Row+role value.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name.

  QVector<MapSelectEntry*> mapListCache; ///< Cached picker rows.

  Q_INVOKABLE int mapToListIndex(int ind); ///< Row index for map @p ind.

public slots:
  void rebuild(); ///< Rebuild the cached list.

public:
  AreaMap* map; ///< The live area map this picker targets.
};
