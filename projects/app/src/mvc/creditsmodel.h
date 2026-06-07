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
#include <QAbstractListModel>

/**
 * @brief Exposes the credits database to QML as a list model -- and the canonical
 *        example of the `mvc/` model pattern.
 *
 * @par The mvc-model convention (shared by every model here)
 * Each is a `QAbstractListModel` that adapts a C++ collection (a DB store or a save
 * object) for a QML `ListView`/`Repeater`. The standard trio is overridden:
 * - **rowCount()** -- how many rows;
 * - **data(index, role)** -- the value for a row + role;
 * - **roleNames()** -- maps the @c *Role enum to the names QML binds to
 *   (`model.name`, `model.url`, ...).
 * Models that wrap a filterable/derived set add a small cache and helper invokables;
 * see SpeciesSelectModel for that variant.
 *
 * @see CreditsDB (the source), Bridge (exposes this as `brg.creditsModel`).
 */
class CreditsModel : public QAbstractListModel
{
  Q_OBJECT

public:
  /// The columns QML can bind to (mapped to names in roleNames()).
  enum RecentFileRoles {
    SectionRole = Qt::UserRole + 1,
    NameRole,
    UrlRole,
    NoteRole,
    LicenseRole,
    MandatedRole
  };

  virtual int rowCount(const QModelIndex& parent) const override;          ///< Number of credit rows.
  virtual QVariant data(const QModelIndex& index, int role) const override; ///< Value for a row + role.
  virtual QHash<int, QByteArray> roleNames() const override;                ///< Role -> QML name map.
};
